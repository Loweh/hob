#ifndef NET_HTTPS_RES_H
#define NET_HTTPS_RES_H

#include <stdlib.h>
#include <string.h>

#include "../util/list.h"
#include "https_hdr.h"

struct https_res {
    int status;
    struct list_node* hdrs;
    char* body;
    int body_sz;
};

struct https_res* https_res_deserialize(char* buf, int sz);
void https_res_free(struct https_res* rs);

int https_res_interpret_line1(char* line, int* status);

#endif