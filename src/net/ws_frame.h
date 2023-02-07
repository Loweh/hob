#ifndef NET_WS_FRAME_H
#define NET_WS_FRAME_H

struct ws_frame {
    int fin;
    int opcode;
    int mask;
    unsigned long length;
    int mask_key;
    char* data;
};

#endif