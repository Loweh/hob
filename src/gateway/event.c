#include "event.h"

struct event* event_deserialize(char* buf, int sz)
{
    jsmn_parser parser;
    jsmn_init(&parser);
    int num_tok = jsmn_parse(&parser, buf, sz, NULL, sz);

    if (num_tok < 1) {
        return NULL;
    }

    jsmn_init(&parser);
    jsmntok_t* tokens = (jsmntok_t*) malloc(sizeof(jsmntok_t) * num_tok);
    
    if (jsmn_parse(&parser, buf, sz, tokens, num_tok) < 1) {
        free(tokens);
        return NULL;
    }

    struct event* e = (struct event*) malloc(sizeof(struct event));
    
    if (event_from_tokens(e, buf, tokens, num_tok)) {
        free(e);
        e = NULL;
    }

    free(tokens);

    return e;
}

void event_free(struct event* e)
{
    if (e->data != NULL) {
        free(e->data);
    }

    free(e);
}

int event_from_tokens(struct event* e, char* buf, jsmntok_t* tokens, int num_tok)
{
    int err = 0;
    jsmntok_t prev;

    for (int i = 0; i < num_tok; i++) {
        if (prev.size != 1 && prev.size != 0) {
            prev = tokens[i];
            continue;
        }

        int prev_sz = prev.end - prev.start;
        int sz = tokens[i].end - tokens[i].start;

        if (prev_sz == 2 && !strncmp(buf + prev.start, "op", 2)) {
            if (tokens[i].type != JSMN_PRIMITIVE) {
                err = -1;
                break;
            }

            char* num = (char*) malloc(sz + 1);
            memcpy(num, buf + tokens[i].start, sz);
            num[sz] = 0;

            e->opcode = atoi(num);
            free(num);
        } else if (prev_sz == 1 && buf[prev.start] == 'd') {
            char* data = (char*) malloc(sz + 1);
            memcpy(data, buf + tokens[i].start, sz);
            data[sz] = 0;
            e->data = data;
        } else if (prev_sz == 1 && buf[prev.start] == 's') {
            if (tokens[i].type != JSMN_PRIMITIVE) {
                err = -2;
                break;
            }

            char* num = (char*) malloc(sz + 1);
            memcpy(num, buf + tokens[i].start, sz);
            num[sz] = 0;

            e->seq = atoi(num);
            free(num);
        }

        prev = tokens[i];
    }

    return err;
}