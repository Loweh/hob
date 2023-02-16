#include "gateway.h"

// This is in dire need of refactoring
struct gateway* gateway_open(char* token)
{
    struct gateway* g = (struct gateway*) malloc(sizeof(struct gateway));
    g->ws = ws_conn_init("ws://gateway.discord.gg", "gateway.discord.gg",
                               "443");

    int err = 0;

    if (!(err = ws_conn_open(g->ws))) {
        printf("Successfully opened WebSocket connection!\n");

        if (!(err = gateway_get_hello(g))) {
            printf("Received Hello event (timeout=%i).\n", g->hb_timeout);

            struct event e;
            e.opcode = 1;
            e.data = NULL;

            if (!gateway_write(g, &e)) {
                struct event* res = NULL;
                int exit = 0;
                int success = 0;

                while (!exit) {
                    int err = gateway_read(g, &res);

                    if (!err && res != NULL) {
                        exit = 1;
                        success = 1;
                    } else if (err < -1) {
                        exit = 1;
                        success = err;
                    }
                }

                if (success == 1) {
                    printf("Received event: opcode: %i, data: %s\n", res->opcode, res->data);
                    event_free(res);
                } else {
                    gateway_close(g);
                    g = NULL;
                    printf("Received error reading heartbeat ACK (%i).", err);
                }
            } else {
                gateway_close(g);
                g = NULL;
                printf("Could not send heartbeat.\n");
            }
        } else {
            gateway_close(g);
            g = NULL;
            printf("Could not receive Hello event (%i)\n", err);
        }
    } else {
        gateway_close(g);
        g = NULL;
        printf("Could not open WebSocket connection (%i).\n", err);
    }

    return g;
}

void gateway_close(struct gateway* g)
{
    ws_conn_close(g->ws);
    free(g);
}

// Refactor this once gateway_read is implemented
int gateway_get_hello(struct gateway* g)
{
    struct ws_frame* f = NULL;
    int exit = 0;

    while (!exit) {
        int res = ws_conn_read(g->ws, &f);

        if (f != NULL && !res) {
            exit = 1;
        } else if (res < 0) {
            return -1;
        }
    }

    struct event* e = event_deserialize(f->data, f->length);
    
    if (e != NULL) {
        int timeout = get_hello_data(e->data);

        if (timeout < 0) {
            event_free(e);
            ws_frame_free(f);
            return -3;
        }

        g->hb_timeout = timeout;

        event_free(e);
    } else {
        return -2;
    }

    ws_frame_free(f);

    return 0;
}

int gateway_read(struct gateway* g, struct event** e)
{
    int res = 0;
    struct ws_frame* f = NULL;

    if ((res = ws_conn_read(g->ws, &f))) {
        return -1;
    }

    if (f != NULL) {
        if (f->opcode == WS_CLOSE_FRAME) {
            ws_frame_free(f);
            return -2;
        } else if (f->opcode != WS_TXT_FRAME) {
            ws_frame_free(f);
            return -3;
        }

        *e = event_deserialize(f->data, f->length);
        ws_frame_free(f);
    }

    return 0;
}

int gateway_write(struct gateway* g, struct event* e)
{
    char* buf = NULL;
    int sz = event_serialize(e, &buf);

    struct ws_frame* f = ws_frame_init(1, WS_TXT_FRAME, 1, sz, 0, buf);
    int result = ws_conn_write(g->ws, f);
    free(buf);

    return result;
}