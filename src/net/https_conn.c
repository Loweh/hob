#include "https_conn.h"

struct https_conn* https_conn_init(char* uri, char* port)
{
    int sz = sizeof(struct https_conn);
    struct https_conn* conn = (struct https_conn*) malloc(sz);
    conn->alive = 0;
    conn->socket = -1;
    conn->ssl = NULL;
    conn->uri = uri;
    conn->port = port;
    
    SSL_CTX* ctx = SSL_CTX_new(SSLv23_client_method());
    int error = 0;

    if (ctx != NULL) {
        conn->ssl = SSL_new(ctx);

        if (conn->ssl != NULL) {
            conn->socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            fcntl(conn->socket, F_SETFL, O_NONBLOCK);
            if (SSL_set_fd(conn->ssl, conn->socket) != 1) {
                error = 1;
            }
        } else {
            error = 1;
        }
    } else {
        error = 1;
    }

    if (error) {
        https_conn_close(conn);
        conn = NULL;
    }

    SSL_CTX_free(ctx);
    return conn;
}

int https_conn_open(struct https_conn* conn)
{
    struct addrinfo* host_info;

    char* host = NULL;
    https_get_host_from_uri(conn->uri, &host);

    if (host == NULL) {
        return -1;
    }

    if (getaddrinfo(host, conn->port, 0, &host_info)) {
        return -2;
    }

    int host_info_sz = sizeof(*host_info->ai_addr);
    if (connect(conn->socket, host_info->ai_addr, host_info_sz)) {
        return -3;
    }


    if (SSL_connect(conn->ssl) != 1) {
        // SSL_connect seems to shutdown itself on error so
        // no need to shutdown here
        return -4;
    }

    if (SSL_do_handshake(conn->ssl) != 1) {
        return -5;
    }

    conn->alive = 1;

    return 0;
}

void https_conn_close(struct https_conn* conn)
{
    if (conn->socket != -1) {
        close(conn->socket);
    }

    if (conn->ssl != NULL) {
        SSL_free(conn->ssl);
    }

    free(conn);
}

void https_get_host_from_uri(char* uri, char** host)
{
    char* proto_sep = strstr(uri, "://");

    if (proto_sep != NULL) {
        int proto_len = proto_sep - uri;
        int length = strlen(uri) - 3 - proto_len;
        *host = (char*) malloc(length + 1);
        memcpy(*host, uri + proto_len + 3, length);
        host[length] = 0;
    }
}