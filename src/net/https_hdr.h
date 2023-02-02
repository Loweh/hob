#ifndef NET_HTTPS_HDR_H
#define NET_HTTPS_HDR_H

#include <stdlib.h>
#include <string.h>

struct https_hdr {
    char* name;
    char* value;
};

int https_hdr_serialize(struct https_hdr hdr, char** buf);

#endif