#include "ready_data.h"

int get_ready_data(char* buf, struct ready_data* d)
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

    jsmntok_t prev;

    for (int i = 0; i < num_tok; i++) {
        if (i) {
            int prev_sz = prev.end - prev.start;

            if (prev_sz == 10 && !strncmp(buf + prev.start, 
                                          "session_id", prev_sz)) {
                int sz = tokens[i].end - tokens[i].start;

                if (tokens[i].type == JSMN_STRING) {
                    char* id = (char*) malloc(sz + 1);
                    memcpy(id, buf + tokens[i].start, sz);
                    id[sz] = 0;

                    d->session_id = id;
                } else {
                    free(tokens);
                    return -1;
                }
            } else if (prev_sz == 18 && !strncmp(buf + prev.start,
                                                "resume_gateway_url", prev_sz)) {
                int sz = tokens[i].end - tokens[i].start;

                if (tokens[i].type == JSMN_STRING) {
                    char* url = (char*) malloc(sz + 1);
                    memcpy(url, buf + tokens[i].start, sz);
                    url[sz] = 0;

                    d->resume_url = url;
                } else {
                    free(tokens);
                    return -2;
                }
            } else if (prev_sz == 11 && !strncmp(buf + prev.start, "application",
                                               prev_sz)) {
                if (tokens[i].type == JSMN_OBJECT) {
                    int sz = tokens[i].end - tokens[i].start;
                    char* app_data = (char*) malloc(sz + 1);
                    memcpy(app_data, buf + tokens[i].start, sz);
                    app_data[sz] = 0;

                    char* app_id = get_app_data(app_data);
                    free(app_data);

                    if (app_id != NULL) {
                        d->app_id = app_id;
                    } else {
                        free(tokens);
                        return -3;
                    }
                }
            }
        }

        prev = tokens[i];
    }

    free(tokens);

    return 0;
}