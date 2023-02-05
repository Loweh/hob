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

struct https_hdr* https_hdr_deserialize(char* buf, int sz)
{
    if (buf == NULL) {
        return NULL;
    }

    int hdr_sz = sizeof(struct https_hdr);
    struct https_hdr* hdr = (struct https_hdr*) malloc(hdr_sz);
    hdr->name = NULL;
    hdr->value = NULL;
    char prev = 0;

    for (int i = 0; i < sz; i++) {
        if (prev == ':' && buf[i] == ' ') {
            int name_sz = i;
            hdr->name = (char*) malloc(name_sz);
            memcpy(hdr->name, buf, name_sz -1);
            hdr->name[name_sz - 1] = 0;

            int value_sz = sz - i + 1;
            hdr->value = (char*) malloc(value_sz);
            memcpy(hdr->value, buf + name_sz + 1, value_sz - 1);
            hdr->value[value_sz - 1] = 0;
        }

        prev = buf[i];
    }

    if (hdr->name == NULL) {
        free(hdr);
        hdr = NULL;
    }

    return hdr;
}

void https_hdr_free(struct https_hdr* hdr)
{
    free(hdr->name);
    free(hdr->value);
    free(hdr);
}