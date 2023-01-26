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

/*
    Populates a struct event with its associated data from the raw JSON in
    the struct ws_frame payload. Returns 0 on success, negative values on
    failure.
*/
int _json_to_event(struct event* e, struct ws_frame* frame,
                   jsmntok_t* tokens, int n);

/*
    Attempts to extract an event from the payload of a given struct ws_frame.
    Returns a struct event allocated dynamically on success, NULL on failure.
*/
struct event* event_deserialize(struct ws_frame* frame);
/*
    Set the payload for a given struct ws_frame to the JSON representation
    of the struct event given. Returns 0 on success, negative value on faliure.
*/
int event_serialize(struct event* e, struct ws_frame* frame);
/*
    Frees the event and its unmanaged resources.
*/
void event_free(struct event** e_ref);

/*
    Get the heartbeat interval from the Gateway hello. Returns the heartbeat
    interval in ms or a negative value on failure.
*/
int get_hello_data(struct event* e);

#endif