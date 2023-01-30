#include "event.h"

int _json_to_event(struct event* e, struct ws_frame* frame, 
                   jsmntok_t* tokens, int n)
{
    int in_main_obj = 0;
    char* last_value = NULL;
    //int last_value_len = 0;
    //jsmntype_t last_type = JSMN_UNDEFINED;

    for (int i = 0; i < n; i++) {
        if (tokens[i].type == JSMN_UNDEFINED) {
            break;
        }
        
        char* value = NULL;
        int len = tokens[i].end - tokens[i].start;

        if (tokens[i].type != JSMN_OBJECT) {
            value = malloc(len);
            memcpy(value, frame->payload + tokens[i].start, len);
        }

        switch (tokens[i].type) {
            case JSMN_OBJECT:
                if (!in_main_obj) {
                    in_main_obj = 1; 
                } else if (!strncmp(last_value, "d", 1)) {
                    e->data = (char*) malloc(len);
                    memcpy(e->data, frame->payload + tokens[i].start, len);
                    e->length = len;
                }
                break;
            case JSMN_PRIMITIVE:
                if (!strncmp(last_value, "op", 2)) {
                    char* num = malloc(len + 1);
                    memcpy(num, frame->payload + tokens[i].start, len);
                    num[len] = 0;
                    char* endptr;

                    e->opcode = (int) strtol(num, &endptr, 10);

                    if (endptr == num) {
                        return -1;
                    }

                    free(num);
                }
                break;
            default:
                break;
        }

        free(last_value);
        last_value = value;
        //last_type = tokens[i].type;
        //last_value_len = len;
    }

    return 0;
}

struct event* event_deserialize(struct ws_frame* frame)
{
    struct event* e = (struct event*) malloc(sizeof(struct event));
    e->opcode = 0;
    e->data = NULL;
    e->seq = 0;
    e->name = NULL;

    jsmn_parser parser;
    jsmntok_t tokens[128] = {{JSMN_UNDEFINED, 0, 0, 0}};
    jsmn_init(&parser);
    

    // Make it dynamic and not just a max 128 tokens
    int ret = jsmn_parse(&parser, frame->payload, frame->length, tokens, 128);

    if (ret > -1) {
            if (_json_to_event(e, frame, tokens, 128)) {
                printf("json_to_event failed\n");
            }
    } else {
        printf("jsmn_parse failed (%i)\n", ret);
    }

    return e;
}

int digit_count(int n)
{
    int digits = 1;

    while (n > 10) {
        n /= 10;
        digits++;
    }

    return digits;
}

struct ws_frame* event_serialize(struct event* e)
{
    struct ws_frame* frame = (struct ws_frame*) malloc(sizeof(struct ws_frame));
    frame->fin = 1;
    frame->opcode = WS_TXT_FRAME;
    frame->mask = 1;
    frame->mask_key = 0;

    char* txt1 = "{\"op\": ";

    int op_digits = digit_count(e->opcode);
    char* opcode = (char*) malloc(op_digits);
    snprintf(opcode, op_digits + 1, "%i", e->opcode);

    char* txt2 = NULL;

    char* seq = NULL;
    int seq_digits = 0;

    if (e->seq != -1) {
        txt2 = ", \"s\": ";
        seq_digits = digit_count(e->seq);
        seq = (char*) malloc(seq_digits);
        snprintf(seq, seq_digits + 1, "%i", e->seq);
    }

    char* txt3 = ", \"d\": ";
    char* txt4 = "}";

    char* segments[7] = {txt1, opcode, txt2, seq, txt3, e->data, txt4};
    int sizes[7] = {strlen(txt1), op_digits, txt2 != NULL ? strlen(txt2) : 0, 
                   seq_digits, strlen(txt3), e->length, strlen(txt4)};

    frame->length = 0;

    for (int i = 0; i < 7; i++) {
        frame->length += sizes[i];
    }

    printf("length: %lu\n", frame->length);

    frame->payload = (char*) malloc(frame->length);
    int offset = 0;

    for (int i = 0; i < 7; i++) {
        if (segments[i] != NULL) {
            memcpy(frame->payload + offset, segments[i], sizes[i]);
            offset += sizes[i];
        }
    }

    return frame;
}

void event_free(struct event** e_ref)
{
    free((*e_ref)->data);
    free(*e_ref);
    *e_ref = NULL;
}

// Consolidate this and _json_to_event into some shared JSON parsing func???
int get_hello_data(struct event* e)
{
    int result = 0;
    char* last_value = NULL;
    int last_value_len = 0;

    jsmn_parser parser;
    jsmntok_t tokens[128] = {{JSMN_UNDEFINED, 0, 0, 0}};
    jsmn_init(&parser);

    int ret = jsmn_parse(&parser, e->data, e->length, tokens, 128);

    if (ret > -1) {
        for (int i = 0; i < 128; i++) {
            if (tokens[i].type == JSMN_UNDEFINED) {
                break;
            }

            char* value = NULL;
            int len = tokens[i].end - tokens[i].start;

            if (tokens[i].type != JSMN_OBJECT) {
                value = malloc(len);
                memcpy(value, e->data + tokens[i].start, len);
            }

            if (last_value != NULL
                && !strncmp("heartbeat_interval", last_value, last_value_len)
                && tokens[i].type == JSMN_PRIMITIVE) {
                char* num = malloc(len + 1);
                memcpy(num, e->data + tokens[i].start, len);
                num[len] = 0;
                char* endptr;

                result = (int) strtol(num, &endptr, 10);

                if (endptr == num) {
                    result = -2;
                    break;
                }

                free(num);
            }

            free(last_value);
            last_value = value;
            last_value_len = len;
        }
    } else {
        result = -1;
    }

    return result;
}