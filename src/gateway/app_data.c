#include "app_data.h"

char* get_app_data(char* buf)
{
    jsmn_parser parser;
    jsmn_init(&parser);

    int buf_sz = strlen(buf);
    int num_tok = jsmn_parse(&parser, buf, buf_sz, NULL, buf_sz);

    if (num_tok < 1) {
        return NULL;
    }

    jsmn_init(&parser);
    jsmntok_t* tokens = (jsmntok_t*) malloc(sizeof(jsmntok_t) * num_tok);
    
    if (jsmn_parse(&parser, buf, buf_sz, tokens, num_tok) < 1) {
        free(tokens);
        return NULL;
    }

    char* result = NULL;
    jsmntok_t prev;

    for (int i = 0; i < num_tok; i++) {
        if (!i) {
            continue;
        }

        int prev_sz = prev.end - prev.start;

        if (prev_sz == 2 && !strncmp(buf + prev.start, "id", prev_sz)) {
            int sz = tokens[i].end - tokens[i].start;

            result = (char*) malloc(sz + 1);
            memcpy(result, buf + tokens[i].start, sz);
            result[sz] = 0;
            break;
        }

        prev = tokens[i];
    }

    free(tokens);
    return result;
}