#ifndef GATEWAY_H
#define GATEWAY_H

#include <stdio.h>
#include <stdlib.h>

#include "../net/ws_conn.h"
#include "event.h"
#include "hello_data.h"

struct gateway {
    struct ws_conn* ws;
    int hb_timeout;
    int hb_last;
};

struct gateway* gateway_open(char* token);
void gateway_close(struct gateway* g);

int gateway_get_hello(struct gateway* g);

#endif