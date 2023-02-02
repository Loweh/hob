#ifndef NET_HTTPS_CONN_H
#define NET_HTTPS_CONN_H

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <errno.h>
#include <poll.h>

#include "https_req.h"

#define HTTPS_BUF_SZ 5120

struct https_conn {
    int alive;
    int socket;
    SSL* ssl;
    char* uri;
    char* port;
};

struct https_conn* https_conn_init(char* uri, char* port);
void https_conn_free(struct https_conn* conn);

int https_conn_open(struct https_conn* conn);
void https_conn_close(struct https_conn* conn);

int https_conn_write(struct https_conn* conn, struct https_req* rq);

void https_get_host_from_uri(char* uri, char** host);

#endif