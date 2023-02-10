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

        struct ws_frame* f = NULL;
        int exit = 0;

        while (!exit) {
            int res = ws_conn_read(ws, &f);

            if (f != NULL && !res) {
                exit = 1;
            }
        }

        printf("Received frame: fin = %i, opcode = %i, mask = %i,"
               "length: %lu, body: %s\n",
                f->fin, f->opcode, f->mask, f->length, f->data);

        ws_frame_free(f);

        struct ws_frame* f2 = ws_frame_init(1, WS_TXT_FRAME, 1, 22, 0,
                                           "{ \"op\": 1, \"d\": null }");
        
        if (!ws_conn_write(ws, f2)) {
            struct ws_frame* f3 = NULL;
            int exit2 = 0;

            while (!exit2) {
                int res = ws_conn_read(ws, &f3);

                if (f3 != NULL && !res) {
                    exit2 = 1;
                }
            }

            printf("Received frame: fin = %i, opcode = %i, mask = %i,"
               "length: %lu, body: %s\n",
                f3->fin, f3->opcode, f3->mask, f3->length, f3->data);
            
            ws_frame_free(f3);
        } else {
            printf("Error writing to WebSocket connection.\n");
        }
    } else {
        printf("Could not open WebSocket connection (%i).\n", err);
    }

    ws_conn_close(ws);
    
    return 0;
}