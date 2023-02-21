#ifndef GATEWAY_H
#define GATEWAY_H

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "../net/ws_conn.h"
#include "event.h"
#include "hello_data.h"
#include "ready_data.h"

struct gateway {
    struct ws_conn* ws;
    char* session_id;
    char* resume_url;
    int seq;
    int hb_timeout;
    time_t hb_last;
};

struct gateway* gateway_open(char* token);
void gateway_close(struct gateway* g);

int gateway_get_hello(struct gateway* g);
int gateway_hb_handshake(struct gateway* g);

int gateway_read(struct gateway* g, struct event** e);
int gateway_write(struct gateway* g, struct event* e);

int gateway_ping(struct gateway* g);
int gateway_identify(struct gateway* g, char* token);
int gateway_listen(struct gateway* g);

#endif