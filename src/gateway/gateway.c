#include "gateway.h"

// This is in dire need of refactoring
struct gateway* gateway_open(char* token)
{
    struct gateway* g = (struct gateway*) malloc(sizeof(struct gateway));
    g->ws = ws_conn_init("ws://gateway.discord.gg", "gateway.discord.gg",
                               "443");
    g->seq = -1;
    g->hb_timeout = 0;
    g->hb_last = 0;

    int err = 0;

    if (!(err = ws_conn_open(g->ws))) {
        printf("Successfully opened WebSocket connection!\n");

        if (!(err = gateway_get_hello(g))) {
            printf("Received Hello event (timeout=%i).\n", g->hb_timeout);

            if (!(err = gateway_hb_handshake(g))) {
                printf("Successfully established Heartbeat.\n");

                if (!(err = gateway_identify(g, "MTA2NjQxNTM3NjM0NDMwMTYxOQ.GJ5Lmv.Q6hCyrQJJU1LnngoJrKJGRnc8GSl_vOLIM209o"))) {
                    printf("Successfully identified to server.\n");
                } else {
                    gateway_close(g);
                    g = NULL;
                    printf("Could not identify to server (%i).\n", err);
                }
            } else {
                gateway_close(g);
                g = NULL;
                printf("Could not establish heartbeat (%i).\n", err);
            }
        } else {
            gateway_close(g);
            g = NULL;
            printf("Could not receive Hello event (%i).\n", err);
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
    struct event* e = NULL;
    int exit = 0;

    while (!exit) {
        int res = gateway_read(g, &e);

        if (e != NULL && !res) {
            exit = 1;
        } else if (res < 0) {
            return -1;
        }
    }
    
    if (e != NULL) {
        int timeout = get_hello_data(e->data);

        if (timeout < 0) {
            event_free(e);
            return -3;
        }

        g->hb_timeout = timeout;

        event_free(e);
    } else {
        return -2;
    }

    return 0;
}

int gateway_hb_handshake(struct gateway* g)
{
    if (!gateway_ping(g)) {
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
            event_free(res);
        } else {
            return -2;
        }
    } else {
        return -1;
    }

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
        int event_seq = (*e)->seq;

        if (event_seq != 0) {
            g->seq = event_seq;
        }

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

int gateway_ping(struct gateway* g)
{
    struct event e;
    e.opcode = 1;
    e.data = NULL;
    
    if (g->seq > -1) {
        int digits = 1;
        int seq = g->seq;

        while (seq >= 10) {
            digits++;
            seq /= 10;
        }

        e.data = (char* ) malloc(digits + 1);
        snprintf(e.data, digits, "%i", seq);
        e.data[digits] = 0;
    }
    
    int err = 0;

    if ((err = gateway_write(g, &e))) {
        return -1;
    }

    g->hb_last = time(NULL);

    return 0;
}

int gateway_identify(struct gateway* g, char* token)
{
    struct event e;
    e.opcode = 2;
    e.data = NULL;

    char* txt1 = "{\"token\": \"";
    int txt1_sz = strlen(txt1);
    char* txt2 = "\", \"intents\": 0, \"properties\": {"
                    "\"os\": \"linux\","
                    "\"browser\": \"hob\","
                    "\"device\": \"hob\""
                 "} }";
    int txt2_sz = strlen(txt2);
    int token_sz = strlen(token);
    
    int sz = txt1_sz + token_sz + txt2_sz + 1;
    e.data = (char*) malloc(sz);
    
    char* segments[4] = {txt1, token, txt2, "\0"};
    int sizes[4] = {txt1_sz, token_sz, txt2_sz, 1};
    int offset = 0;

    for (int i = 0; i < 4; i++) {
        memcpy(e.data + offset, segments[i], sizes[i]);
        offset += sizes[i];
    }

    if (gateway_write(g, &e)) {
        free(e.data);
        return -1;
    }

    free(e.data);

    struct event* res = NULL;
    int exit = 0;
    int success = 0;

    while (!exit) {
        int err = gateway_read(g, &res);

        if (!err && res != NULL) {
            exit = 1;
            success = 1;
        } else if (err != 0) {
            exit = 1;
            success = err;
        }
    }

    if (success == 1) {
        printf("Received event: opcode: %i, data: %s\n", res->opcode, res->data);
        event_free(res);
    } else {
        return -2;
    }

    return 0;
}