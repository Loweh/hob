#include "hello_data.h"

int get_hello_data(char* buf)
{
    jsmn_parser parser;
    jsmn_init(&parser);
    int buf_sz = strlen(buf);
    int num_tok = jsmn_parse(&parser, buf, buf_sz, NULL, buf_sz);

    if (num_tok < 1) {
        return -1;
    }

    jsmn_init(&parser);
    jsmntok_t* tokens = (jsmntok_t*) malloc(sizeof(jsmntok_t) * num_tok);
    
    if (jsmn_parse(&parser, buf, buf_sz, tokens, num_tok) < 1) {
        free(tokens);
        return -2;
    }

    int heartbeat = 0;
    jsmntok_t prev;

    for (int i = 0; i < num_tok; i++) {
        if (i) {
            int prev_sz = prev.end - prev.start;

            if (prev_sz == 18 && !strncmp(buf + prev.start, 
                                          "heartbeat_interval", prev_sz)) {
                int sz = tokens[i].end - tokens[i].start;

                if (tokens[i].type == JSMN_PRIMITIVE) {
                    char* num = (char*) malloc(sz + 1);
                    memcpy(num, buf + tokens[i].start, sz);
                    num[sz] = 0;

                    heartbeat = atoi(num);
                    free(num);
                    break;
                } else {
                    heartbeat = -3;
                    break;
                }
            }
        }

        prev = tokens[i];
    }

    free(tokens);

    return heartbeat;
}