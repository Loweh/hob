#ifndef NET_WS_CONN_H
#define NET_WS_CONN_H

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <openssl/rand.h>
#include <openssl/evp.h>

#include "https_conn.h"
#include "ws_frame.h"

#define WS_KEY_BYTE_SZ 16 // WebSocket key size for HTTP handshake
#define WS_KEY_SZ 25 // +1 for the null terminator

struct ws_conn {
    int alive;
    char* host;
    unsigned char* key;
    struct https_conn* https;
};

struct ws_conn* ws_conn_init(char* uri, char* host, char* port);

int ws_conn_open(struct ws_conn* conn);
void ws_conn_close(struct ws_conn* conn);

int ws_conn_write(struct ws_conn* conn, struct ws_frame* f);
int ws_conn_read(struct ws_conn* conn, struct ws_frame** f);

int ws_conn_handshake_send(struct ws_conn* conn);
int ws_conn_handshake_receive(struct ws_conn* conn);
int ws_conn_handshake_verify_hdrs(struct ws_conn* conn, struct https_res* rs);
int ws_conn_handshake_verify_accept(char* key, char* accept);

#endif