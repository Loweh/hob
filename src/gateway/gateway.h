#ifndef GATEWAY_H
#define GATEWAY_H

#include <stdlib.h>

#include "../net/conn.h"
#include "event.h"

/*
    Gateway will have read and write functions that take / return dynamically
    allocated struct events.
*/

struct gateway {
    int alive;
    struct conn* c;
    char* token;
    int token_sz;
    int timeout;
    int last_ping;
    int evt_seq;
};

/*
    Initializes the components necessary for a Discord Gateway API connection.
    Returns a dynamically allocated struct gateway.
*/
struct gateway* gateway_init(char* token, int token_sz);
/*
    Frees the components associated with a Gateway connection.
*/
void gateway_free(struct gateway** g);

/*
    Opens the Gateway connection and performs the necessary handshakes.
    Returns 0 on success, negative on failure.
*/
int gateway_open(struct gateway* g);
/*
    Closes the Gateway connection.
*/
void gateway_close(struct gateway* g);

/*
    Identifies the bot to the Gateway server. Returns 0 on success, negative
    values on failure.
*/
int gateway_identify(struct gateway* g);
/*
    Sends a Gateway Heartbeat message. Returns the return value of SSL_write.
*/
int gateway_ping(struct gateway* g);

int get_num_str_len(int num);

#endif
