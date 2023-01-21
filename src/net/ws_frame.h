#ifndef NET_WS_FRAME_H
#define NET_WS_FRAME_H

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <openssl/rand.h>
#include <openssl/evp.h>

enum ws_opcode {
    WS_CONT_FRAME = 0,
    WS_TXT_FRAME = 1,
    WS_BIN_FRAME = 2,
    WS_CLOSE_FRAME = 8,
    WS_PING_FRAME = 9,
    WS_PONG_FRAME = 10
};

struct ws_frame {
    unsigned char fin; // Is the final frame in the message
    enum ws_opcode opcode;
    unsigned char mask; // Is a mask being used on the payload
    unsigned long length; // Payload length
    unsigned int mask_key; // Masking key
    char* payload;
};

char* reverse_bytes(char* src, int n);
struct ws_frame* ws_deserialize_frame(char* raw, int n);
int ws_serialize_frame(struct ws_frame* frame, char** data);

#endif