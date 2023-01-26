#include <stdio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "net/conn.h"
#include "net/ws_frame.h"
#include "net/http.h"
#include "gateway/gateway.h"

int main()
{
    /*
    struct http_rq* rq = http_rq_init(HTTP_GET, "/", 1);
    http_rq_add_hdr(rq, "Origin", 6, "example.com", 11);
    http_rq_add_hdr(rq, "Test1", 5, "ABCDEFGHIJKLMNOP", 16);
    char* str = NULL;
    int len = http_rq_serialize(rq, &str);
    printf("%s", str);
    */

    /*
    char* str = "HTTP/1.1 200 OK\r\nDate: aaaaaaaaaaaa\r\nOrigin: example.com\r\n";
    int strlen = 59;

    struct http_rs* rs = http_rs_deserialize(str, strlen);

    //free(str);
    //http_rq_free(rq);
    http_rs_free(rs);
    */

    struct gateway* g = gateway_init();
    
    if (!gateway_open(g)) {
        gateway_ping(g);

        struct ws_frame* out1_frame = conn_read(g->c);
        printf("Received frame:\n\tfin: %hhu\n\topcode: %hhu\n\t"
               "mask: %hhu\n\tlength: %lu\n\tmask_key: %u\n\tpayload: %s\n",
               out1_frame->fin, out1_frame->opcode, out1_frame->mask,
               out1_frame->length, out1_frame->mask_key, out1_frame->payload);
        free(out1_frame);

        struct ws_frame in_frame;
        in_frame.fin = 1;
        in_frame.opcode = WS_TXT_FRAME;
        in_frame.mask = 1;
        in_frame.mask_key = 0;
        in_frame.length = 178;
        in_frame.payload = "{\"op\": 2, \"d\":" 
            "{ \"token\": \"MTA2NjQxNTM3NjM0NDMwMTYxOQ.G89LZy.qM_07NR_iWkaT7iLQrj9xa5qwfwU5YZcf4sZXY\", \"properties\": {"
                "\"os\": \"linux\","
                "\"browser\": \"hob\","
                "\"device\": \"hob\"},"
            "\"intents\": 0"
            "}"
        "}";

        printf("payload: %s\n", in_frame.payload);

        conn_write(g->c, &in_frame);

        out1_frame = conn_read(g->c);
        printf("Received frame:\n\tfin: %hhu\n\topcode: %hhu\n\t"
               "mask: %hhu\n\tlength: %lu\n\tmask_key: %u\n\tpayload: %s\n",
               out1_frame->fin, out1_frame->opcode, out1_frame->mask,
               out1_frame->length, out1_frame->mask_key, out1_frame->payload);
        free(out1_frame);

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