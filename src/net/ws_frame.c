#include "ws_frame.h"

struct ws_frame* ws_frame_init(int fin, enum ws_opcode opcode, int mask,
                               unsigned long length, int mask_key, char* data)
{
    if ((fin != 0 && fin != 1) || (mask != 0 && mask != 1)) {
        return NULL;
    }

    int sz = sizeof(struct ws_frame);
    struct ws_frame* f = (struct ws_frame*) malloc(sz);
    f->fin = fin;
    f->opcode = opcode;
    f->mask = mask;
    f->length = length;
    f->mask_key = mask_key;
    f->data = data;
    return f;
}

void ws_frame_free(struct ws_frame* f)
{
    // If this is a frame received from the server, free the dynamically
    // allocated data.
    if (!f->mask) {
        free(f->data);
    }

    free(f);
}

struct ws_frame* ws_frame_deserialize(char* buf, int sz)
{
    int offset = 0;

    unsigned char fin = (buf[offset] & 0b10000000) >> 7;
    unsigned char opcode = buf[offset] & 0b00001111;
    offset++;

    unsigned char len_init = buf[offset] & 0b01111111;
    offset++;

    unsigned long length = 0;

    if (len_init < 126) {
        length = (unsigned long) len_init;
    } else if (len_init == 126) {
        memcpy(&length, buf + offset, 2);
        length = (unsigned long) ntohs((uint16_t) length);
        offset += 2;
    } else {
        memcpy(&length, buf + offset, 8);
        length = ntohll(length);
        offset += 8;
    }

    char* data = (char*) malloc(length);
    memcpy(data, buf + offset, length);

    // Server to client communication never uses a mask, thus, no need to
    // deserialize its related fields.
    return ws_frame_init((int) fin, (enum ws_opcode) opcode, 0, length, 0, data);
}

int ws_frame_serialize(struct ws_frame* f, char** buf)
{
    return 0;
}


unsigned long ntohll(unsigned long n)
{
    return 0;
}