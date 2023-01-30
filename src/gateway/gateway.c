#include "gateway.h"

struct gateway* gateway_init(char* token, int token_sz)
{
    struct gateway* g = (struct gateway*) malloc(sizeof(struct gateway));
    g->alive = 0;
    g->c = conn_init("gateway.discord.gg", "443", "wss://gateway.discord.gg");
    g->token = token;
    g->token_sz = token_sz;
    g->timeout = 0;
    g->last_ping = 0;
    g->evt_seq = 0;
    return g;
}

void gateway_free(struct gateway** g)
{
    struct gateway* gw = *g;

    if (gw->alive) {
        gateway_close(gw);
    }

    free(gw);
    *g = NULL;
}

int gateway_open(struct gateway* g)
{
    int result = 0;

    if (!conn_open(g->c)) {
        g->alive = 1;

        if (!conn_handshake(g->c)) {
            // Get hello event from server
            struct ws_frame* out_frame = conn_read(g->c);
            printf("Received frame:\n\tfin: %hhu\n\topcode: %hhu\n\t"
               "mask: %hhu\n\tlength: %lu\n\tmask_key: %u\n\tpayload: %s\n",
               out_frame->fin, out_frame->opcode, out_frame->mask,
               out_frame->length, out_frame->mask_key, out_frame->payload);
            struct event* e = event_deserialize(out_frame);

            if (e->opcode != EVENT_HELLO) {
                gateway_close(g);
                result = -3;
            } else {
                int hb_interval = get_hello_data(e);
                printf("Heartbeat interval: %d\n", hb_interval);
                g->timeout = hb_interval;

                gateway_ping(g);
                struct ws_frame* out_frame = conn_read(g->c);
                printf("Received message from ping: %s\n", out_frame->payload);

                printf("Identifying...\n");
                gateway_identify(g);
            }

            event_free(&e);
            ws_free_frame(&out_frame);
        } else {
            gateway_close(g);
            result = -2;
        }
    } else {
        result = -1;
    }

    return result;
}

void gateway_close(struct gateway* g)
{
    conn_close(&(g->c));
    g->alive = 0;
}

int gateway_identify(struct gateway* g)
{
    struct event identify;
    identify.opcode = EVENT_ID;
    identify.seq = -1;

    char* txt1 = "{ \"token\": \"";
    int txt1_sz = strlen(txt1);
    char* txt2 = "\", \"properties\": {"
                        "\"os\": \"linux\","
                        "\"browser\": \"hob\","
                        "\"device\": \"hob\"},"
                        "\"intents\": 0"
                    "}";
    int txt2_sz = strlen(txt2);

    identify.length = txt1_sz + txt2_sz + g->token_sz;
    identify.data = (char*) malloc(identify.length);
    int offset = 0;

    memcpy(identify.data + offset, txt1, txt1_sz);
    offset += txt1_sz;
    memcpy(identify.data + offset, g->token, g->token_sz);
    offset += g->token_sz;
    memcpy(identify.data + offset, txt2, txt2_sz);

    struct ws_frame* in_frame = event_serialize(&identify);

    conn_write(g->c, in_frame);
    free(identify.data);
    free(in_frame);

    // Wait for a response, make sure it is a READY event before returning 0.
    // Grab important stuff like session ID as well

    struct ws_frame* out_frame = conn_read(g->c);
    printf("Received message from identify: %s\n", out_frame->payload);

    return 0;
}

int gateway_ping(struct gateway* g)
{
    struct ws_frame frame;
    frame.fin = 1;
    frame.opcode = WS_TXT_FRAME;
    frame.mask = 1;
    frame.mask_key = 0;

    if (g->evt_seq != 0) {
        char* payld1 = "{\"op\": 1, \"d\": ";
        int payld1_len = strlen(payld1);
        int evt_seq_len = get_num_str_len(g->evt_seq);
        frame.length = payld1_len + evt_seq_len + 1;
        frame.payload = (char*) malloc(frame.length);
        
        memcpy(frame.payload, payld1, payld1_len);
        snprintf(frame.payload + payld1_len, evt_seq_len + 2, "%i}", g->evt_seq);
        printf("payload to send: %s\n", frame.payload);
    } else {
        frame.length = 22;
        frame.payload = (char*) malloc(frame.length);
        memcpy(frame.payload, "{ \"op\": 1, \"d\": null }", frame.length);
    }

    int ret = conn_write(g->c, &frame);
    free(frame.payload);
    return ret;
}

int get_num_str_len(int num)
{
    int n = 1;

    while (num > 10) {
        num /= 10;
        n++;
    }

    return n;
}
