#include "https_hdr.h"

int https_hdr_serialize(struct https_hdr hdr, char** buf)
{
    int name_sz = strlen(hdr.name);
    int value_sz = strlen(hdr.value);
    int sz = name_sz + value_sz + 4;

    *buf = (char*) malloc(sz);
    int offset = 0;

    memcpy(*buf + offset, hdr.name, name_sz);
    offset += name_sz;
    memcpy(*buf + offset, ": ", 2);
    offset += 2;
    memcpy(*buf + offset, hdr.value, value_sz);
    offset += value_sz;
    memcpy(*buf + offset, "\r\n", 2);
    offset += 2;

    return sz;
}