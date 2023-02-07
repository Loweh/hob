#include <stdio.h>
#include <openssl/rand.h>
#include "net/ws_conn.h"

int main()
{
    struct ws_conn* ws = ws_conn_init("ws://gateway.discord.gg",
                                      "gateway.discord.gg", "443");
    int err = 0;

    if (!(err = ws_conn_open(ws))) {
        printf("Successfully opened WebSocket connection!\n");
    } else {
        printf("Could not open WebSocket connection (%i).\n", err);
    }

    ws_conn_close(ws);
    /*
    struct https_conn* conn = https_conn_init("https://example.com", "443");
    int result = https_conn_open(conn);
    if (!result) {
        struct https_req* rq = https_req_init(HTTPS_GET, "/", NULL, 0);
        https_req_add_hdr(rq, "Host", "example.com");
        https_req_add_hdr(rq, "Connection", "Upgrade");
        https_req_add_hdr(rq, "Upgrade", "websocket");
        https_req_add_hdr(rq, "Sec-WebSocket-Version", "13");

        unsigned char key[WS_KEY_SZ] = {0};
        generate_ws_key(key);
        https_req_add_hdr(rq, "Sec-WebSocket-Key", (char*) key);

        int ret = https_conn_write(conn, rq);
        if (!ret) {
            struct https_res* rs = NULL;
            int exit = 0;

            while (!exit) {
                int err = https_conn_read(conn, &rs);

                if (rs != NULL && !err && rs->body_sz == rs->body_sz_read) {
                    printf("HTTPS Response Code: %i\n", rs->status);
                    struct list_node* node = rs->hdrs;

                    while (node != NULL) {
                        struct https_hdr* hdr = (struct https_hdr*) node->value;
                        printf("Header (name=%s) (value=%s)\n", hdr->name, hdr->value);
                        node = node->next;
                    }

                    printf("Body (%i): %s\n", rs->body_sz, rs->body);

                    https_res_free(rs);
                    rs = NULL;
                    exit = 1;
                } else {
                    if (err != 0) {
                        printf("Error while reading from https_conn (%i).", err);
                    }
                }
            }
        } else {
            printf("ERROR READING\n");
        }

        https_conn_close(conn);
    } else {
        printf("%i\n", result);
        https_conn_close(conn);
    }

    */
    
    return 0;
}