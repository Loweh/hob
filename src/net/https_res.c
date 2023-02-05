#include "https_res.h"

struct https_res* https_res_deserialize(char* buf, int sz)
{
    if (buf == NULL) {
        return NULL;
    }

    int start = 0;
    int line_cnt = 0;
    char prev = 0;

    // Make into its own function
    for (int i = 0; i < sz; i++) {
        if (prev == '\r' && buf[i] == '\n') {
            int line_sz = i - start - 1;
            if (line_sz > 0) {
                start = i + 1;
                line_cnt++;
            } else {
                i = sz;
            }
        }

        prev = buf[i];
    }

    char** lines = (char**) malloc(sizeof(char*) * line_cnt);

    // Make into its own function
    start = 0;
    prev = 0;
    int valid_lines = 0;

    for (int i = 0; i < sz; i++) {
        if (prev == '\r' && buf[i] == '\n') {
            int line_sz = i - start - 1;
            if (line_sz > 0) {
                char* line = (char*) malloc(line_sz + 1);
                memcpy(line, buf + start, line_sz);
                memcpy(line + line_sz, "\0", 1);
                lines[valid_lines] = line;
                start = i + 1;
                valid_lines++;
            } else {
                break;
            }
             
        }

        prev = buf[i];
    }

    int res_sz = sizeof(struct https_res);
    struct https_res* rs = (struct https_res*) malloc(res_sz);
    int error = 0;
    
    if (!https_res_interpret_line1(lines[0], &rs->status)) {
        for (int i = 1; i < valid_lines; i++) {
            if (https_res_add_hdr(rs, lines[i])) {
                break;
                error = 1;
            }
        }
    } else {
        error = 1;
    }

    if (error) {
        free(rs);
        rs = NULL;
    }
 
    for (int i = 0; i < valid_lines; i++) {
        free(lines[i]);
    }

    free(lines);

    struct list_node* node = rs->hdrs;
    int has_content = 0;

    while (node != NULL) {
        struct https_hdr* hdr = (struct https_hdr*) node->value;

        if (!strcmp(hdr->name, "Content-Length")) {
            has_content = 1;
            rs->body_sz = atoi(hdr->value);
        }

        node = node->next;
    }

    if (!has_content) {
        rs->body_sz = 0;
    }

    rs->body = NULL;
    rs->body_sz_read = 0;
    
    return rs;
}

void https_res_free(struct https_res* rs)
{
    list_free(rs->hdrs, (void (*)(void*)) https_hdr_free);
    
    if (rs->body != NULL) {
        free(rs->body);
    }
    
    free(rs);
}

int https_res_interpret_line1(char* line, int* status)
{
    int line_len = strlen(line);
    char* version_end = strstr(line, " ");

    if (version_end == NULL) {
        return -1;
    }

    int version_sz = strstr(line, " ") - line - 1;

    if (strncmp(line, "HTTP/1.1", version_sz)) {
        return -3;
    }

    if (line_len < 12) {
        return -4;
    }

    // Status codes are 3 characters long
    char* s = (char*) malloc(sizeof(3));
    memcpy(s, version_end + 1, 3);
    *status = atoi(s);
    free(s);

    return 0;
}

int https_res_add_hdr(struct https_res* rs, char* line)
{
    struct https_hdr* hdr = https_hdr_deserialize(line, strlen(line));

    if (hdr != NULL) {
        struct list_node* node = list_node_init(hdr);

        if (rs->hdrs == NULL) {
            rs->hdrs = node;
        } else {
            struct list_node* cur = rs->hdrs;

            while (cur->next != NULL) {
                cur = cur->next;
            }

            cur->next = node;
        }
    } else {
        return -1;
    }

    return 0;
}

int https_res_read_body(struct https_res* rs, char* buf, int sz)
{
    if (rs->body_sz < 1) {
        return -1;
    }

    int body_sz_rem = rs->body_sz - rs->body_sz_read;
    int len = body_sz_rem < sz ? body_sz_rem : sz;

    if (rs->body == NULL) {
        rs->body = (char*) malloc(rs->body_sz);
        memcpy(rs->body, buf, len);
        rs->body_sz_read = len;
    } else {
        if (rs->body_sz_read >= rs->body_sz) {
            return -2;
        }

        memcpy(rs->body + rs->body_sz_read, buf, len);
        rs->body_sz_read += len;
    }

    return 0;
}
