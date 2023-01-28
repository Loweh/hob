#include "conn.h"

struct conn* conn_init(char* hostname, char* port, char* path)
{
    struct conn* c = (struct conn*) malloc(sizeof(struct conn));
    c->state = CONN_CLOSED;
    c->hostname = hostname;
    c->port = port;
    c->path = path;
    c->last_ping = 0;

    SSL_CTX* ctx = SSL_CTX_new(SSLv23_client_method());
    int error = 0;

    if (ctx != NULL) {
        c->ssl = SSL_new(ctx);

        if (c->ssl != NULL) {
            c->socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            error = SSL_set_fd(c->ssl, c->socket) == 1 ? 0 : 1;
        } else {
            error = 1;
        }
    } else {
        error = 1;
    }

    if (error) {
        free(c);
        c = NULL;
    }

    return c;
}

int conn_open(struct conn* c)
{
    int result = 0;

    struct addrinfo* h_info = NULL;

    if (!getaddrinfo(c->hostname, c->port, 0, &h_info)) {
        if (!connect(c->socket, h_info->ai_addr, sizeof(*h_info->ai_addr))) {
            if (SSL_connect(c->ssl) != -1) {
                result = SSL_do_handshake(c->ssl) == 1 ? 0 : -4;
                c->state = !result ? CONN_OPEN : CONN_CLOSED;
            } else {
                // SSL_connect seems to shutdown itself on error so
                // no need to shutdown here
                result = -3;
            }
        } else {
            result = -2;
        }
    } else {
        result = -1;
    }

    return result;
}

void conn_close(struct conn** c)
{
    if ((*c)->state == CONN_OPEN) {
        SSL_shutdown((*c)->ssl);
        close((*c)->socket);
        free(*c);
        *c = NULL;
    }
}

int conn_handshake(struct conn* c)
{
    int result = 0;

    if (c->state == CONN_OPEN) {
        char* str = NULL;
        // Generate the base64 key
        unsigned char key[25] = {0};
        unsigned char bytes[CONN_WS_KEY_SZ] = {0};

        if (RAND_bytes(bytes, CONN_WS_KEY_SZ) != 1) {
            return -2;
        }

        int key_sz = EVP_EncodeBlock(key, bytes, CONN_WS_KEY_SZ);
        
        struct http_rq* rq = http_rq_init(HTTP_GET, c->path, strlen(c->path));
        int err = http_rq_add_hdr(rq, "Host", 4, c->hostname, strlen(c->hostname));
        err += http_rq_add_hdr(rq, "Connection", 10, "Upgrade", 7);
        err += http_rq_add_hdr(rq, "Upgrade", 7, "websocket", 9);
        err += http_rq_add_hdr(rq, "Sec-WebSocket-Version", 21, "13", 2);
        err += http_rq_add_hdr(rq, "Sec-WebSocket-Key", 17, (char*) key, key_sz);

        if (err != 0) {
            return -3;
        }

        int str_sz = http_rq_serialize(rq, &str);
        printf("%s\n", str);
        SSL_write(c->ssl, str, str_sz);
        http_rq_free(rq);

        // TODO: VERIFY RESPONSE
        // Adjust this buffer size?
        char buf[4096] = {0};
        SSL_read(c->ssl, buf, 4096);
        printf("%s\n", buf);
        int res = _conn_check_handshake_response(buf, 4096, key, key_sz);
        
        if (res != 0) {
            result = -2;
        }
    } else {
        result = -1;
    }

    return result;
}

// TODO: Clean this up
int _conn_check_handshake_response(
    char* str,
    int str_len,
    unsigned char* key,
    int key_len)
{
    int result = 0;

    struct http_rs* rs = http_rs_deserialize(str, str_len);
    
    if (!strncmp(rs->version, "HTTP/1.1", rs->version_sz)) {
        if (rs->status == 101) {
            int has_conn = 0, has_upgrade = 0, has_accept = 0;

            struct http_hdr* cur = http_get_hdr(rs->hdrs, "Connection", 10);

            if (cur != NULL) {
                _strnlower(cur->value, cur->value_sz);
                int min_sz = cur->value_sz < 7 ? cur->value_sz : 7;

                if (!strncmp(cur->value, "upgrade", min_sz)) {
                    has_conn = 1;
                }
            }
            
            cur = http_get_hdr(rs->hdrs, "Upgrade", 7);

            if (cur != NULL) {
                _strnlower(cur->value, cur->value_sz);
                int min_sz = cur->value_sz < 9 ? cur->value_sz : 9;

                if (!strncmp(cur->value, "websocket", min_sz)) {
                    has_upgrade = 1;
                }
            }

            cur = http_get_hdr(rs->hdrs, "Sec-WebSocket-Accept", 20);

            if (cur != NULL) {
                char* magic_str = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
                int magic_len = strlen(magic_str);
                char* concat = (char*) malloc(key_len + magic_len);
                memcpy(concat, key, key_len);
                memcpy(concat + key_len, magic_str, magic_len);

                EVP_MD_CTX* ctx = EVP_MD_CTX_new();

                if (ctx != NULL) {
                    // Do checks for errors on these
                    EVP_DigestInit_ex(ctx, EVP_sha1(), NULL);
                    EVP_DigestUpdate(ctx, concat, magic_len + key_len);
                    unsigned char buf[64] = {0};
                    unsigned int buf_len = 64;
                    EVP_DigestFinal_ex(ctx, buf, &buf_len);

                    unsigned char buf2[64] = {0};
                    EVP_EncodeBlock(buf2, buf, 20);

                    if (!strncmp(buf2, cur->value, cur->value_sz)) {
                        has_accept = 1;
                    }
                }

                EVP_MD_CTX_free(ctx);
            }

            if (!has_conn || !has_upgrade || !has_accept) {
                result = -3;
            }
        } else {
            result = -2;
        }
    } else {
        result = -1;
    }

    http_rs_free(rs);

    return result;
}

struct ws_frame* conn_read(struct conn* c)
{
    // Discord payloads are meant to be 4096 bytes or less. Add an extra 1KB
    // now for WebSocket header.
    char buf[5120] = {0};
    SSL_read(c->ssl, buf, 5120);
    return ws_deserialize_frame(buf, 5120);
}

int conn_write(struct conn* c, struct ws_frame* frame)
{
    char* out = NULL;
    int out_len = ws_serialize_frame(frame, &out);
    int result = SSL_write(c->ssl, out, out_len);
    free(out);
    return result;
}