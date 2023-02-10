#include "ws_conn.h"

struct ws_conn* ws_conn_init(char* uri, char* host, char* port)
{
    int sz = sizeof(struct ws_conn);
    struct ws_conn* conn = (struct ws_conn*) malloc(sz);
    conn->alive = 0;
    conn->host = host;
    conn->key = NULL;
    conn->https = https_conn_init(uri, port);

    if (conn->https == NULL) {
        free(conn);
        conn = NULL;
    }

    return conn;
}

int ws_conn_open(struct ws_conn* conn)
{
    if (!https_conn_open(conn->https)) {
        if (ws_conn_handshake_send(conn)) {
            return -2;
        }

        if (ws_conn_handshake_receive(conn)) {
            return -3;
        }
    } else {
        return -1;
    }

    conn->alive = 1;

    return 0;
}

void ws_conn_close(struct ws_conn* conn)
{
    conn->alive = 0;

    if (conn->key != NULL) {
        free(conn->key);
    }

    if (conn->https != NULL) {
        https_conn_close(conn->https);
    }

    free(conn);
}

int ws_conn_write(struct ws_conn* conn, struct ws_frame* f)
{
    char* buf = NULL;
    int buf_sz = ws_frame_serialize(f, &buf);
    int res = 0;
    int result = 0;

    while ((res = SSL_write(conn->https->ssl, buf, buf_sz)) <= 0) {
        int err = SSL_get_error(conn->https->ssl, res);

        if (err != SSL_ERROR_WANT_READ && err != SSL_ERROR_WANT_WRITE) {
            result = -1;
            break;
        }
    }

    ws_frame_free(f);
    free(buf);

    return result;
}

int ws_conn_read(struct ws_conn* conn, struct ws_frame** f)
{
    char buf[HTTPS_BUF_SZ] = {0};
    int res = 0;

    if ((res = SSL_read(conn->https->ssl, buf, HTTPS_BUF_SZ)) > 0) {
        *f = ws_frame_deserialize(buf, HTTPS_BUF_SZ);
    } else {
        int err = SSL_get_error(conn->https->ssl, res);
        if (err != SSL_ERROR_WANT_READ && err != SSL_ERROR_WANT_WRITE) {
            return -1;
        }
    }

    return 0;
}

int ws_conn_handshake_send(struct ws_conn* conn)
{
    unsigned char bytes[WS_KEY_BYTE_SZ] = {0};

    if (RAND_bytes(bytes, WS_KEY_BYTE_SZ) != 1) {
        return -1;
    }

    conn->key = (unsigned char*) malloc(WS_KEY_SZ);
    conn->key[WS_KEY_SZ - 1] = 0;

    // Since we know there will always be WS_KEY_BYTE_SZ bytes, there is no need
    // to store the key size return value, since EVP_EncodeBlock will thus also
    // always return a set number of bytes (WS_KEY_SZ bytes to be exact).
    EVP_EncodeBlock(conn->key, bytes, WS_KEY_BYTE_SZ);

    struct https_req* rq = https_req_init(HTTPS_GET, conn->https->uri, NULL, 0);
    https_req_add_hdr(rq, "Host", conn->host);
    https_req_add_hdr(rq, "Connection", "Upgrade");
    https_req_add_hdr(rq, "Upgrade", "websocket");
    https_req_add_hdr(rq, "Sec-WebSocket-Key", (char*) conn->key);
    https_req_add_hdr(rq, "Sec-WebSocket-Version", "13");

    int ret = https_conn_write(conn->https, rq);

    if (ret) {
        return -2;
    }

    return 0;
}

int ws_conn_handshake_receive(struct ws_conn* conn)
{
    struct https_res* rs = NULL;
    int exit = 0;

    while (!exit) {
        int ret = https_conn_read(conn->https, &rs);

        if (rs != NULL && !ret) {
            exit = 1;
        } else if (ret) {
            return -1;
        }
    }

    if (rs->status != 101) {
        https_res_free(rs);
        return -2;
    }

    if (ws_conn_handshake_verify_hdrs(conn, rs)) {
        https_res_free(rs);
        return -3;
    }

    https_res_free(rs);
    return 0;
}

int ws_conn_handshake_verify_hdrs(struct ws_conn* conn, struct https_res* rs)
{
    int valid_upgrade, valid_conn, valid_accept;
    struct list_node* node = rs->hdrs;

    while (node != NULL) {
        struct https_hdr* hdr = (struct https_hdr*) node->value;
        
        if (!strcmp(hdr->name, "upgrade")) {
            int val_sz = strlen(hdr->value);
            char* lower_value = (char*) malloc(val_sz + 1);

            for (int i = 0; i < val_sz; i++) {
                lower_value[i] = tolower(hdr->value[i]);
            }

            if (!strcmp(lower_value, "websocket")) {
                valid_upgrade = 1;
            }

            free(lower_value);
        } else if (!strcmp(hdr->name, "connection")) {
            int val_sz = strlen(hdr->value);
            char* lower_value = (char*) malloc(val_sz + 1);
            
            for (int i = 0; i < val_sz; i++) {
                lower_value[i] = tolower(hdr->value[i]);
            }

            if (!strcmp(hdr->value, "upgrade")) {
                valid_conn = 1;
            }

            free(lower_value);
        } else if (!strcmp(hdr->name, "sec-websocket-accept")) {
            if (!ws_conn_handshake_verify_accept((char*) conn->key,
                                                 hdr->value)) {
                valid_accept = 1;
            }
        }
        node = node->next;
    }

    if (!valid_upgrade || !valid_conn || !valid_accept) {
        return -1;
    }

    return 0;
}

int ws_conn_handshake_verify_accept(char* key, char* accept)
{
    char* magic_str = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    int magic_len = strlen(magic_str);
    int key_len = strlen(key);

    char* concat = (char*) malloc(key_len + magic_len);
    memcpy(concat, key, key_len);
    memcpy(concat + key_len, magic_str, magic_len);

    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    int result = 0;

    if (ctx != NULL) {
        if (EVP_DigestInit_ex(ctx, EVP_sha1(), NULL)) {
            if (EVP_DigestUpdate(ctx, concat, magic_len + key_len)) {
                unsigned int buf_len = EVP_MD_CTX_size(ctx);
                unsigned char* buf = (unsigned char*) malloc(buf_len);

                if (EVP_DigestFinal_ex(ctx, buf, &buf_len)) {
                    unsigned char buf2[32] = {0};
                    EVP_EncodeBlock(buf2, buf, 20);

                    if (strncmp((char*) buf2, accept, strlen(accept))) {
                        result = -4;
                    }
                } else {
                    result = -3;
                }

                free(buf);
            } else {
                result = -2;
            }
        } else {
            result = -1;
        }
    }

    free(concat);
    EVP_MD_CTX_free(ctx);
    return result;
}