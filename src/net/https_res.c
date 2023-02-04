#include "https_res.h"

struct https_res* https_res_deserialize(char* buf, int sz)
{
    if (buf == NULL) {
        return NULL;
    }

    int start = 0;
    int line_cnt = 0;
    char prev = 0;

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
    
    if (!https_res_interpret_line1(lines[0], &rs->status)) {
        // Line 1 is valid, now interpret headers.
    } else {
        free(rs);
        rs = NULL;
    }

    for (int i = 0; i < valid_lines; i++) {
        free(lines[i]);
    }

    free(lines);

    return rs;
}

void https_res_free(struct https_res* rs)
{
    //list_free(rs->hdrs, (void (*)(void*)) https_hdr_free);
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
