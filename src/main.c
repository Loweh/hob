#include <stdio.h>
#include <openssl/rand.h>
#include "net/https_conn.h"
#include "net/https_req.h"
#include "net/https_res.h"

#define WS_KEY_BYTE_SZ 16 // WebSocket key size for HTTP handshake
#define WS_KEY_SZ 25 // +1 for the null terminator

int generate_ws_key(unsigned char* key);

int main()
{
    
    /*
    struct https_hdr* hdr = https_hdr_deserialize("Content-Type: application-json", 30);
    printf("hdr name: %s, hdr value: %s\n", hdr->name, hdr->value);
    https_hdr_free(hdr);
    */
    struct https_conn* conn = https_conn_init("https://example.com", "443");
    int result = https_conn_open(conn);
    if (!result) {
        struct https_req* rq = https_req_init(HTTPS_GET, "/", NULL);
        https_req_add_hdr(rq, "Host", "example.com");
        /*
        https_req_add_hdr(rq, "Connection", "Upgrade");
        https_req_add_hdr(rq, "Upgrade", "websocket");
        https_req_add_hdr(rq, "Sec-WebSocket-Version", "13");

        unsigned char key[WS_KEY_SZ] = {0};
        generate_ws_key(key);
        https_req_add_hdr(rq, "Sec-WebSocket-Key", (char*) key);
        */

        int ret = https_conn_write(conn, rq);
        if (!ret) {
            char buf2[4096] = {0};
            while (SSL_read(conn->ssl, buf2, 4096) <= 0) {}
            printf("buf: %s\n", buf2);
            struct https_res* rs = https_res_deserialize(buf2, 4096);
            
            if (rs != NULL) {
                printf("HTTPS Response Code: %i\n", rs->status);

                struct list_node* node = rs->hdrs;

                while (node != NULL) {
                    struct https_hdr* hdr = (struct https_hdr*) node->value;
                    printf("Header (name=%s) (value=%s)\n", hdr->name, hdr->value);
                    node = node->next;
                }

                https_res_free(rs);
            }

            while (rs->body_sz_read < rs->body_sz) {
                while (SSL_read(conn->ssl, buf2, 256) <= 0) {}
                https_res_read_body(rs, buf2, 256);
            }

            printf("Body (%i): %s\n", rs->body_sz, rs->body);
        } else {
            printf("ERROR READING\n");
        }

        https_conn_close(conn);
    } else {
        printf("%i\n", result);
        https_conn_close(conn);
    }
    
    return 0;
}

int generate_ws_key(unsigned char* key)
{
    unsigned char bytes[WS_KEY_BYTE_SZ] = {0};

    if (RAND_bytes(bytes, WS_KEY_BYTE_SZ) != 1) {
        return -1;
    }

    // Since we know there will always be WS_KEY_BYTE_SZ bytes, there is no need
    // to store the key size return value, since EVP_EncodeBlock will thus also
    // always return a set number of bytes (WS_KEY_SZ bytes to be exact).
    EVP_EncodeBlock(key, bytes, WS_KEY_BYTE_SZ);
    return 0;
}