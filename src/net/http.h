#ifndef NET_HTTP_H
#define NET_HTTP_H

#include <stdlib.h>
#include <string.h>

enum http_mth {
    HTTP_GET,
    HTTP_POST
};

struct http_hdr {
    char* name;
    int name_sz;
    char* value;
    int value_sz;
    void* next;
};

struct http_rq {
    enum http_mth method;
    char* path;
    int path_sz;
    char* version;
    int version_sz;
    struct http_hdr* hdrs;
};

struct http_rs {
    char* version;
    int version_sz;
    int status;
    struct http_hdr* hdrs;
};

struct http_hdr* http_get_hdr(struct http_hdr* hdrs, char* name, int name_sz);

struct http_rq* http_rq_init(enum http_mth method, char* path, int path_sz);
void http_rq_free(struct http_rq* rq);

int http_rq_add_hdr(struct http_rq* rq, char* name, int name_sz, char* value,
                    int value_sz);

int http_rq_serialize(struct http_rq* rq, char** str);

struct http_rs* http_rs_init(char* version, int version_sz, int status);
void http_rs_free(struct http_rs* rs);

struct http_rs* http_rs_deserialize(char* str, int sz);
void _http_rs_deserialize_first(struct http_rs* rs, char* line, int line_sz);
void _http_rs_deserialize_hdr(struct http_rs* rs, char* line, int line_sz);

#endif