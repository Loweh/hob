#include <stdio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "net/conn.h"
#include "net/ws_frame.h"
#include "gateway/gateway.h"

int main()
{
    struct gateway* g = gateway_init();
    
    if (!gateway_open(g)) {
        while (1) {
            printf("Sending ping...\n");
            gateway_ping(g);
        
            struct ws_frame* out_frame = conn_read(g->c);
            printf("Received frame:\n\tfin: %hhu\n\topcode: %hhu\n\t"
                   "mask: %hhu\n\tlength: %lu\n\tmask_key: %u\n\tpayload: %s\n",
                   out_frame->fin, out_frame->opcode, out_frame->mask,
                   out_frame->length, out_frame->mask_key, out_frame->payload);

            sleep(5);
        }
        
    } else {
        printf("Error: could not open gateway connection.\n");
    }

    gateway_free(&g);
    /*
    struct conn* c = conn_init(
        "gateway.discord.gg",
        "443",
        "wss://gateway.discord.gg");

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

            conn_write(c, &frame);

            struct ws_frame* out_frame = conn_read(c);

            printf("received frame:\n\tfin: %hhu\n\topcode: %hhu\n\t"
                   "mask: %hhu\n\tlength: %lu\n\tmask_key: %u\n\tpayload: %s\n",
                   out_frame->fin, out_frame->opcode, out_frame->mask,
                   out_frame->length, out_frame->mask_key, out_frame->payload);

            struct event* e = event_deserialize(out_frame);

            printf("received event:\n\topcode: %d\n\tdata: %s\n", e->opcode,
                   e->data);

            event_free(&e);
            ws_free_frame(&out_frame);

            conn_close(&c);
        } else {
            conn_close(&c);
            printf("ERROR: Could not open connection. (%d)\n", result);
        }
    } else {
        printf("ERROR: Could not initialize connection.\n");
    }
    */

    return 0;
}