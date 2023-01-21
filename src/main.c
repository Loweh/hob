#include <stdio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "net/conn.h"
#include "net/ws_frame.h"

int main()
{
    struct conn* c = conn_init(
        "demo.piesocket.com",
        "443",
        "wss://demo.piesocket.com/v3/channel_123?api_key=VCXCEuvhGcBDP7XhiJJUDv"
        "R1e1D3eiVjgZ9VRiaV&notify_self");

    if (c != NULL) {
        int result = conn_open(c);
        if (!result) {
            conn_handshake(c);

            struct ws_frame frame;
            frame.fin = 1;
            frame.opcode = WS_PING_FRAME;
            frame.mask = 1;
            frame.length = 0;
            frame.mask_key = 0;
            frame.payload = NULL;

            char* out = NULL;
            int out_len = ws_serialize_frame(&frame, &out);
            SSL_write(c->ssl, out, out_len);

            char buf[2048] = {0};
            SSL_read(c->ssl, buf, 2048);
            struct ws_frame* out_frame = ws_deserialize_frame(buf, 2048);

            printf("received frame:\n\tfin: %hhu\n\topcode: %hhu\n\t"
                   "mask: %hhu\n\tlength: %lu\n\tmask_key: %u\n\tpayload: %s\n",
                   out_frame->fin, out_frame->opcode, out_frame->mask,
                   out_frame->length, out_frame->mask_key, out_frame->payload);
            /*
            char* out = "GET / HTTP/1.1\r\nHost: example.com\r\n\r\n";
            SSL_write(c->ssl, out, 38);

            int exit = 0;

            while (!exit) {
                char in[2048] = {0};
                int result = SSL_read(c->ssl, in, 2048);

                if (result > 0) {
                    printf("Received from server:\n%s\n", in);
                } else {
                    if (SSL_get_error(c->ssl, result) != SSL_ERROR_WANT_READ) {
                        printf("ERROR: Fatal error reading from connection.\n");
                        exit = 1;
                    }
                }
            }
            */

            conn_close(&c);
        } else {
            conn_close(&c);
            printf("ERROR: Could not open connection. (%d)\n", result);
        }
    } else {
        printf("ERROR: Could not initialize connection.\n");
    }

    return 0;
}