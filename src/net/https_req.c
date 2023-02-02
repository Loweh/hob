#include "https_req.h"

struct https_req* https_req_init(enum https_mth method, char* path, char* body)
{
    int sz = sizeof(struct https_req);
    struct https_req* rq = (struct https_req*) malloc(sz);
    rq->method = method;
    rq->path = path;
    rq->version = " HTTP/1.1\r\n";
    rq->hdrs = NULL;
    rq->body = body;
    return rq;
}

void https_req_free(struct https_req* rq)
{
    list_free(rq->hdrs, value_free_default);
    free(rq);
}

void https_req_add_hdr(struct https_req* rq, char* name, char* value)
{
    int hdr_sz = sizeof(struct https_hdr);
    struct https_hdr* hdr = (struct https_hdr*) malloc(hdr_sz);
    hdr->name = name;
    hdr->value = value;

    if (rq->hdrs == NULL) {
        rq->hdrs = list_node_init(hdr);
    } else {
        struct list_node* node = rq->hdrs;

        while (node->next != NULL) {
            node = node->next;
        }
        
        node->next = list_node_init(hdr);
    }
}

int https_req_serialize(struct https_req* rq, char** buf)
{
    int mth_sz = 0;
    char* mth_txt = NULL;

    if (rq->method == HTTPS_GET) {
        mth_sz += 4;
        mth_txt = "GET ";
    } else {
        mth_sz += 5;
        mth_txt = "POST ";
    }

    int path_sz = strlen(rq->path);
    int version_sz = strlen(rq->version);
    int hdrs_sz = 0;
    int hdr_cnt = list_length(rq->hdrs);

    char** lines = NULL;
    int* sizes = NULL;

    if (hdr_cnt > 0) {
        lines = (char**) malloc(sizeof(char*) * hdr_cnt);
        sizes = (int*) malloc(sizeof(int) * hdr_cnt);

        struct list_node* node = rq->hdrs;
        int i = 0;

        while (node != NULL) {
            struct https_hdr* hdr = (struct https_hdr*) node->value;
            char* buf = NULL;
            int buf_sz = https_hdr_serialize(*hdr, &buf);
            lines[i] = buf;
            sizes[i] = buf_sz;
            hdrs_sz += buf_sz;
            i++;
            node = node->next;
        }
    }
    
    int sz = mth_sz + path_sz + version_sz + hdrs_sz;
    *buf = (char*) malloc(sz);
    int offset = 0;

    memcpy(*buf + offset, mth_txt, mth_sz);
    offset += mth_sz;
    memcpy(*buf + offset, rq->path, path_sz);
    offset += path_sz;
    memcpy(*buf + offset, rq->version, version_sz);
    offset += version_sz;

    for (int i = 0; i < hdr_cnt; i++) {
        memcpy(*buf + offset, lines[i], sizes[i]);
        offset += sizes[i];
        free(lines[i]);
    }

    if (lines != NULL && sizes != NULL) {
        free(lines);
        free(sizes);
    }

    return sz;
}