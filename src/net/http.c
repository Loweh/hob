#include "http.h"

struct http_hdr* http_get_hdr(struct http_hdr* hdrs, char* name, int name_sz)
{
    struct http_hdr* cur = hdrs;

    while (cur != NULL) {
        if (name_sz == cur->name_sz) {
            if (!strncmp(name, cur->name, name_sz)) {
                break;
            }
        }
    }

    return cur;
}

struct http_rq* http_rq_init(enum http_mth method, char* path, int path_sz)
{
    int rq_sz = sizeof(struct http_rq);
    struct http_rq* rq = (struct http_rq*) malloc(rq_sz);
    rq->method = method;
    rq->path_sz = path_sz;
    rq->path = path;
    rq->version = "HTTP/1.1";
    rq->version_sz = 8;
    rq->hdrs = NULL;
    return rq;
}

void http_rq_free(struct http_rq* rq)
{
    while (rq->hdrs != NULL) {
        struct http_hdr* h = rq->hdrs;
        rq->hdrs = rq->hdrs->next;
        free(h);
    }

    free(rq);
}

int http_rq_add_hdr(struct http_rq* rq, char* name, int name_sz, char* value,
                    int value_sz)
{
    if (name == NULL || name_sz < 1) {
        return -1;
    } else if (value == NULL || value_sz < 1) {
        return -2;
    }

    struct http_hdr* hdr = (struct http_hdr*) malloc(sizeof(struct http_hdr));
    hdr->name = name;
    hdr->name_sz = name_sz;
    hdr->value = value;
    hdr->value_sz = value_sz;
    hdr->next = NULL;

    if (rq->hdrs == NULL) {
        rq->hdrs = hdr;
    } else {
        struct http_hdr* last = rq->hdrs;

        while (last != NULL) {
            // Make sure there are no duplicate headers
            if (last->name_sz == name_sz) {
                if (!strncmp(name, last->name, name_sz)) {
                    return -3;
                }
            }
            
            if (last->next != NULL) {
                last = last->next;
            } else {
                break;
            }
        }

        last->next = hdr;
    }

    return 0;
}

int http_rq_serialize(struct http_rq* rq, char** str)
{
    int hdr_cnt = 0;
    struct http_hdr* cur = rq->hdrs;

    while (cur != NULL) {
        hdr_cnt++;
        cur = cur->next;
    }

    // 6 counts for the first line, with 3 segments separated by 2 spaces and
    // terminated with \r\n, 4 counts for each header for the name, colon
    // separator, the value, then a terminating \r\n. Finally a +1 for the 
    // trailing \r\n.
    int seg_cnt = 6 + hdr_cnt * 4 + 1;
    char** segments = (char**) malloc(sizeof(char*) * seg_cnt);
    int* sizes = (int*) malloc(sizeof(int) * seg_cnt);
    int cur_seg = 0;

    char* first_line[6] = {rq->method == HTTP_GET ? "GET" : "POST", " ",
                           rq->path, " ", rq->version, "\r\n"};
    int first_sizes[6] = {rq->method == HTTP_GET ? 3 : 4, 1, rq->path_sz, 1,
                          rq->version_sz, 2};

    for (int i = 0; i < 6; i++) {
        segments[cur_seg] = first_line[i];
        sizes[cur_seg] = first_sizes[i];
        cur_seg++;
    }

    cur = rq->hdrs;

    while (cur != NULL) {
        char* line[4] = {cur->name, ": ", cur->value, "\r\n"};
        int size[4] = {cur->name_sz, 2, cur->value_sz, 2};

        for (int i = 0; i < 4; i++) {
            segments[cur_seg] = line[i];
            sizes[cur_seg] = size[i];
            cur_seg++;
        }

        cur = cur->next;
    }

    segments[cur_seg] = "\r\n";
    sizes[cur_seg] = 2;
    
    int total_sz = 0;

    for (int i = 0; i < seg_cnt; i++) {
        total_sz += sizes[i];
    }

    *str = (char*) malloc(total_sz);
    int offset = 0;

    for (int i = 0; i < seg_cnt; i++) {
        memcpy(*str + offset, segments[i], sizes[i]);
        offset += sizes[i];
    }

    free(segments);
    free(sizes);

    return total_sz;
}

struct http_rs* http_rs_init(char* version, int version_sz, int status)
{
    struct http_rs* rs = (struct http_rs*) malloc(sizeof(struct http_rs));
    rs->version = version;
    rs->version_sz = version_sz;
    rs->status = status;
    return rs;
}

void http_rs_free(struct http_rs* rs)
{
    while (rs->hdrs != NULL) {
        struct http_hdr* h = rs->hdrs;
        rs->hdrs = rs->hdrs->next;
        free(h->name);
        free(h->value);
        free(h);
    }

    free(rs->version);
    free(rs);
}

struct http_rs* http_rs_deserialize(char* str, int sz)
{
    int line_cnt = 0;
    char prev = 0;

    for (int i = 0; i < sz; i++) {
        if (prev == '\r' && str[i] == '\n') {
            line_cnt++;
        }

        prev = str[i];
    }

    char** lines = (char**) malloc(sizeof(char*) * line_cnt);
    int* sizes = (int*) malloc(sizeof(int) * line_cnt);
    int cur_line = 0;
    int start = 0, is_new_line = 1;

    for (int i = 0; i < sz; i++) {
        if (is_new_line) {
            start = i;
            is_new_line = 0;
        } else if (prev == '\r' && str[i] == '\n') {
            // Subtract 1 so the previous \r is not included in the line
            int length = i - start - 1;

            if (length > 0) {
                char* line = (char*) malloc(length);
                memcpy(line, str + start, length);
                lines[cur_line] = line;
                sizes[cur_line] = length;

                cur_line++;
                start = i + 1;
            } 
        }

        prev = str[i];
    }

    struct http_rs* rs = (struct http_rs*) malloc(sizeof(struct http_rs));

    for (int i = 0; i < line_cnt; i++) {
        if (!i) {
            _http_rs_deserialize_first(rs, lines[i], sizes[i]);
        } else {
            _http_rs_deserialize_hdr(rs, lines[i], sizes[i]);
        }
    }

    for (int i = 0; i < line_cnt; i++) {
        free(lines[i]);
    }

    free(lines);
    free(sizes);

    return rs;
}

void _http_rs_deserialize_first(struct http_rs* rs, char* line, int line_sz)
{
    int start = 0;

    for (int i = 0; i < line_sz; i++) {
        if (line[i] == ' ') {
            if (start == 0) {
                rs->version = (char*) malloc(i);
                memcpy(rs->version, line, i);
                rs->version_sz = i;
            } else {
                rs->status = strtol(line + start, line + i - start - 1, 10);
                break;
            }
            start = i + 1;
        }
    }
}

void _http_rs_deserialize_hdr(struct http_rs* rs, char* line, int line_sz)
{
    struct http_hdr* hdr = (struct http_hdr*) malloc(sizeof(struct http_hdr));
    char prev = 0;

    for (int i = 0; i < line_sz; i++) {
        if (prev == ':' && line[i] == ' ') {
            int name_sz = i - 1;
            char* name = (char*) malloc(name_sz);
            memcpy(name, line, name_sz);
            
            int value_sz = line_sz - i - 1;
            char* value = (char*) malloc(value_sz);
            memcpy(value, line + i + 1, value_sz);

            hdr->name = name;
            hdr->name_sz = name_sz;
            hdr->value = value;
            hdr->value_sz = value_sz;

            break;
        }

        prev = line[i];
    }

    if (rs->hdrs != NULL) {
        struct http_hdr* cur = rs->hdrs;

        while (cur->next != NULL) {
            cur = cur->next;
        }

        cur->next = hdr;
    } else {
        rs->hdrs = hdr;
    }
}