#include "ws_conn.h"

struct ws_conn* ws_conn_init(char* uri, char* host, char* port)
{
    int sz = sizeof(struct ws_conn);
    struct ws_conn* conn = (struct ws_conn*) malloc(sz);
    conn->alive = 0;
    conn->host = host;
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
        // Put into separate function
        unsigned char key[WS_KEY_SZ] = {0};
        unsigned char bytes[WS_KEY_BYTE_SZ] = {0};

        if (RAND_bytes(bytes, WS_KEY_BYTE_SZ) != 1) {
            return -2;
        }

        // Since we know there will always be WS_KEY_BYTE_SZ bytes, there is no need
        // to store the key size return value, since EVP_EncodeBlock will thus also
        // always return a set number of bytes (WS_KEY_SZ bytes to be exact).
        EVP_EncodeBlock(key, bytes, WS_KEY_BYTE_SZ);

        struct https_req* rq = https_req_init(HTTPS_GET, conn->https->uri, NULL, 0);
        https_req_add_hdr(rq, "Host", conn->host);
        https_req_add_hdr(rq, "Connection", "Upgrade");
        https_req_add_hdr(rq, "Upgrade", "websocket");
        https_req_add_hdr(rq, "Sec-WebSocket-Key", (char*) key);
        https_req_add_hdr(rq, "Sec-WebSocket-Version", "13");

        int ret = https_conn_write(conn->https, rq);

        if (ret) {
            return -2;
        }

        
        // Put into separate function
        struct https_res* rs = NULL;
        int exit = 0;

        while (!exit) {
            int ret = https_conn_read(conn->https, &rs);

            if (rs != NULL && !ret) {
                exit = 1;
            } else if (ret) {
                printf("rs = %i\n", rs);
                return -3;
            }
        }

        if (rs->status != 101) {
            return -4;
        }

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
                valid_accept = 1;
            }

            node = node->next;
        }

        if (!valid_upgrade || !valid_conn || !valid_accept) {
            return -5;
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

    if (conn->https != NULL) {
        https_conn_close(conn->https);
    }

    free(conn);
}