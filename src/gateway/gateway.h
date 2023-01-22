#ifndef GATEWAY_H
#define GATEWAY_H

#include <stdlib.h>

#include "../net/conn.h"

/*
    Gateway will have read and write functions that take / return dynamically
    allocated struct events.
*/

struct gateway {
    int alive;
    struct conn* c;
    int timeout;
};

struct gateway* gateway_init();
void gateway_free(struct gateway** g);

int gateway_open(struct gateway* g);
void gateway_close(struct gateway* g);

int gateway_ping();

#endif
