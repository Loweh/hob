#ifndef GATEWAY_EVENT_H
#define GATEWAY_EVENT_H
#define JSMN_HEADER

#include <stdlib.h>
#include <string.h>

#include "../ext/jsmn.h"

struct event {
    int opcode;
    int seq;
    char* name;
    char* data;
};

struct event* event_deserialize(char* buf, int sz);
void event_free(struct event* e);

int event_from_tokens(struct event* e, char* buf, jsmntok_t* tokens, int num_tok);

#endif