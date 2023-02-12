#include "gateway.h"

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
        } else {
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