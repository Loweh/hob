#include "gateway.h"

// This is in dire need of refactoring
struct gateway* gateway_open(char* token)
{
    struct gateway* g = (struct gateway*) malloc(sizeof(struct gateway));
    g->ws = ws_conn_init("ws://gateway.discord.gg", "gateway.discord.gg",
                               "443");
    g->seq = -1;
    g->hb_timeout = 0;
    g->app_id = NULL;
    g->session_id = NULL;
    g->resume_url = NULL;
    g->hb_last = 0;

    int err = 0;

    if (!(err = ws_conn_open(g->ws))) {
        printf("Successfully opened WebSocket connection!\n");

        if (!(err = gateway_get_hello(g))) {
            printf("Received Hello event (timeout=%i).\n", g->hb_timeout);

            if (!(err = gateway_hb_handshake(g))) {
                printf("Successfully established Heartbeat.\n");

                if (!(err = gateway_identify(g, token))) {
                    printf("Successfully identified to server.\n\tapp_id: %s\n"
                           "\tsession_id: %s\n\tresume_url: %s\n",
                           g->app_id, g->session_id, g->resume_url);

                    printf("Starting gateway listening.\n");
                    int err = gateway_listen(g);
                    if (err) {
                        printf("Error while listening to gateway (%i).\n", err);
                    }
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
    if (g->app_id != NULL) {
        free(g->app_id);
    }

    if (g->session_id != NULL) {
        free(g->session_id);
    }
    
    if (g->resume_url != NULL) {
        free(g->resume_url);
    }

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
        snprintf(e.data, digits + 1, "%i", g->seq);
        e.data[digits] = 0;
    }
    
    int err = 0;

    if ((err = gateway_write(g, &e))) {
        free(e.data);
        return -1;
    }

    free(e.data);

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
        struct ready_data data;

        if (get_ready_data(res->data, &data)) {
            event_free(res);
            return -3;
        }

        g->app_id = data.app_id;
        g->session_id = data.session_id;
        g->resume_url = data.resume_url;

        event_free(res);
    } else {
        return -2;
    }

    return 0;
}

int gateway_listen(struct gateway* g)
{
    int exit = 0;

    while (!exit) {
        time_t curtime = time(NULL);
        time_t diff = curtime - g->hb_last;

        if (diff >= g->hb_timeout / 1000) {
            printf("\tSending heartbeat (lastping=%i).\n", (int) diff);

            if (gateway_ping(g)) {
                exit = -1;
            }
        }

        struct event* e = NULL;
        int err = gateway_read(g, &e);
        
        if (!err && e != NULL) {
            printf("\tReceived event:\n\t\topcode: %i\n\t\tseq: %i\n\t\tdata: %s\n",
                   e->opcode, e->seq, e->data);
            event_free(e);
        } else if (err) {
            exit = -2;
        }
    }

    return exit;
}