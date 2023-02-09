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
        
        char buf[5120] = {0};
        while (SSL_read(ws->https->ssl, buf, 5120) <= 0) {}

        struct ws_frame* f = ws_frame_deserialize(buf, 5120);
        printf("Received frame: fin = %i, opcode = %i, mask = %i,"
               "length: %lu, body: %s\n",
                f->fin, f->opcode, f->mask, f->length, f->data);

        ws_frame_free(f);

        struct ws_frame* f2 = ws_frame_init(1, WS_TXT_FRAME, 1, 22, 0,
                                           "{ \"op\": 1, \"d\": null }");
        char* buf2 = NULL;
        int len = ws_frame_serialize(f, &buf2);
        SSL_write(ws->https->ssl, buf2, len);

        free(buf2);
        ws_frame_free(f2);

        while (SSL_read(ws->https->ssl, buf, 5120) <= 0) {}

        struct ws_frame* f3 = ws_frame_deserialize(buf, 5120);
        printf("Received frame: fin = %i, opcode = %i, mask = %i,"
               "length: %lu, body: %s\n",
                f3->fin, f3->opcode, f3->mask, f3->length, f3->data);
            
        ws_frame_free(f3);
    } else {
        printf("Could not open WebSocket connection (%i).\n", err);
    }

    ws_conn_close(ws);
    
    return 0;
}