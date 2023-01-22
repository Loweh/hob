#ifndef NET_CONN_H
#define NET_CONN_H

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <openssl/ssl.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/sha.h>

#include "ws_frame.h"

#define CONN_WS_KEY_SZ 16 // WebSocket key size for HTTP handshake

enum conn_state {
    CONN_OPEN,
    CONN_CLOSED
};

struct conn {
    enum conn_state state;
    int socket;
    SSL* ssl;
    char* hostname;
    char* port;
    char* path;
    int last_ping;  
};

/*
    Creates a new SSL connection associated with the given hostname, port
    and resource path. Returns NULL on failure.
*/
struct conn* conn_init(char* hostname, char* port, char* path);
/*
    Opens a connection. Returns 0 on success, negative values on failure.
*/
int conn_open(struct conn* c);
/*
    Closes the connection and frees its resources.
*/
void conn_close(struct conn** c);

/*
    Performs the WebSocket HTTP handshake. Returns 0 on success, negative values
    on failure.
*/
int conn_handshake(struct conn* c);
/*
    Helper function for conn_handshake. Generates initial HTTP handshake
    message. Returns the length in bytes of the message or -1 on error.
*/
int _conn_assemble_handshake(struct conn* c, char** r, unsigned char* key);
/*
    Helper function for conn_handshake. Interpets the response HTTP message
    to ensure it is valid. Returns 0 on success, negative values on failure.
*/
int _conn_check_handshake_response(
    char* rs,
    int rs_len,
    unsigned char* key,
    int key_len);

/*
    Reads from the connection. Returns a struct ws_frame on success and NULL
    on error.
*/
struct ws_frame* conn_read(struct conn* c);

/*
    Writes a struct ws_frame to the connection. Returns the result of SSL_write.
*/
int conn_write(struct conn* c, struct ws_frame* frame);

#endif