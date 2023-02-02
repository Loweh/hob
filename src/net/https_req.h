#ifndef NET_HTTPS_REQ_H
#define NET_HTTPS_REQ_H

#include "../util/list.h"
#include "https_hdr.h"

enum https_mth {
    HTTPS_GET,
    HTTPS_POST
};

struct https_req {
    enum https_mth method;
    char* path;
    char* version;
    struct list_node* hdrs;
    char* body;
};

struct https_req* https_req_init(enum https_mth method, char* path, char* body);
void https_req_free(struct https_req* rq);

void https_req_add_hdr(struct https_req* rq, char* name, char* value);

int https_req_serialize(struct https_req* rq, char** buf);

#endif