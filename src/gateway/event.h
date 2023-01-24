#ifndef GATEWAY_EVENT_H
#define GATEWAY_EVENT_H

#include <stdlib.h>
#include <string.h>

#define JSMN_HEADER
#include "../ext/jsmn.h"
#include "../net/ws_frame.h"

enum event_opcode {
    EVENT_DISPATCH = 0,
    EVENT_HB = 1,
    EVENT_ID = 2,
    EVENT_RESUME = 6,
    EVENT_RECONNECT = 7,
    EVENT_SESS_INVALID = 9,
    EVENT_HELLO = 10,
    EVENT_HB_ACK = 11
};

struct event {
    enum event_opcode opcode;
    char* data;
    int length;
    unsigned int seq;
    char* name;
};

int _json_to_event(struct event* e, struct ws_frame* frame,
                   jsmntok_t* tokens, int n);

struct event* event_deserialize(struct ws_frame* frame);
int event_serialize(struct event* e, struct ws_frame* frame);
void event_free(struct event** e_ref);

int get_hello_data(struct event* e);

#endif