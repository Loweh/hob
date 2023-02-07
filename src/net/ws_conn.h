#ifndef NET_WS_CONN_H
#define NET_WS_CONN_H

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <openssl/rand.h>
#include "https_conn.h"

#define WS_KEY_BYTE_SZ 16 // WebSocket key size for HTTP handshake
#define WS_KEY_SZ 25 // +1 for the null terminator

struct ws_conn {
    int alive;
    char* host;
    struct https_conn* https;
};

struct ws_conn* ws_conn_init(char* uri, char* host, char* port);

int ws_conn_open(struct ws_conn* conn);
void ws_conn_close(struct ws_conn* conn);

#endif