#include <stdio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "net/conn.h"
#include "net/ws_frame.h"
#include "net/http.h"
#include "gateway/gateway.h"

int main()
{
    struct gateway* g = gateway_init("MTA2NjQxNTM3NjM0NDMwMTYxOQ.G89LZy.qM_07NR_iWkaT7iLQrj9xa5qwfwU5YZcf4sZXY", 72);
    
    if (!gateway_open(g)) {
        while (1) {
            printf("Sending ping...\n");
            gateway_ping(g);
        
            struct ws_frame* out_frame = conn_read(g->c);
            printf("Received frame:\n\tfin: %hhu\n\topcode: %hhu\n\t"
                   "mask: %hhu\n\tlength: %lu\n\tmask_key: %u\n\tpayload: %s\n",
                   out_frame->fin, out_frame->opcode, out_frame->mask,
                   out_frame->length, out_frame->mask_key, out_frame->payload);
            free(out_frame);

            sleep(g->timeout / 1000);
        }
        
    } else {
        printf("Error: could not open gateway connection.\n");
    }

    gateway_free(&g);

    return 0;
}