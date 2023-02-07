#ifndef NET_WS_FRAME_H
#define NET_WS_FRAME_H

#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

enum ws_opcode {
    WS_CONT_FRAME = 0,
    WS_TXT_FRAME = 1,
    WS_BIN_FRAME = 2,
    WS_CLOSE_FRAME = 8,
    WS_PING_FRAME = 9,
    WS_PONG_FRAME = 10
};

struct ws_frame {
    int fin;
    enum ws_opcode opcode;
    int mask;
    unsigned long length;
    int mask_key;
    char* data;
};

struct ws_frame* ws_frame_init(int fin, enum ws_opcode opcode, int mask,
                               unsigned long length, int mask_key, char* data);
void ws_frame_free(struct ws_frame* f);

struct ws_frame* ws_frame_deserialize(char* buf, int sz);
int ws_frame_serialize(struct ws_frame* f, char** buf);

unsigned long ntohll(unsigned long n);

#endif