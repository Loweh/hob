#include "gateway.h"

struct gateway* gateway_init()
{
    struct gateway* g = (struct gateway*) malloc(sizeof(struct gateway));
    g->alive = 0;
    g->c = NULL;
    g->timeout = 0;
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
    return 0;
}

void gateway_close(struct gateway* g)
{
    
}

