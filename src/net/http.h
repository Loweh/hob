#ifndef NET_HTTP_H
#define NET_HTTP_H

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

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

void _strnlower(char* str, int n);
/*
    Attempts to find a given header in an list of headers given a name.
    Returns a struct http_hdr* on success, NULL on failure.
*/
struct http_hdr* http_get_hdr(struct http_hdr* hdrs, char* name, int name_sz);


/*
    Dynamically allocates a new struct http_rq. Returns the newly allocated
    struct.
*/
struct http_rq* http_rq_init(enum http_mth method, char* path, int path_sz);
/*
    Frees the given struct http_rq.
*/
void http_rq_free(struct http_rq* rq);


/*
    Adds a new header with the given name and value to the given struct
    http_rq. Returns 0 on success, negative values on failure.
*/
int http_rq_add_hdr(struct http_rq* rq, char* name, int name_sz, char* value,
                    int value_sz);

int http_rq_serialize(struct http_rq* rq, char** str);

struct http_rs* http_rs_init(char* version, int version_sz, int status);
void http_rs_free(struct http_rs* rs);

struct http_rs* http_rs_deserialize(char* str, int sz);
void _http_rs_deserialize_first(struct http_rs* rs, char* line, int line_sz);
void _http_rs_deserialize_hdr(struct http_rs* rs, char* line, int line_sz);

#endif