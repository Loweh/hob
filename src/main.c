#include <stdio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "net/conn.h"

int main()
{
    struct conn* c = conn_init("demo.piesocket.com", 18, "443", 3, "wss://demo.piesocket.com/v3/channel_123?api_key=VCXCEuvhGcBDP7XhiJJUDvR1e1D3eiVjgZ9VRiaV&notify_self", 100);

    if (c != NULL) {
        int result = conn_open(c);
        if (!result) {
            conn_handshake(c);
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