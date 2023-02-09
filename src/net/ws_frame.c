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
    int sz = 2;

    char byte1 = f->fin ? 0b10000000 : 0;
    byte1 |= f->opcode;

    char byte2 = f->mask ? 0b10000000 : 0;
    char* length = NULL;
    int len_sz = f->length > 0xFFFF ? 8 : 2;

    if (f->length < 126) {
        byte2 |= f->length;
    } else {
        byte2 |= f->length > 0xFFFF ? 127 : 126;

        sz += len_sz;
        length = (char*) malloc(len_sz);
        memcpy(length, &f->length, len_sz);
    }

    char* mask_key = NULL;
    int mask_key_sz = sizeof(f->mask_key);

    if (f->mask) { 
        mask_key = (char*) malloc(mask_key_sz);
        memcpy(mask_key, &f->mask_key, mask_key_sz);
        sz += mask_key_sz;
    }

    sz += f->length;

    // Implement XORing of data
    *buf = (char*) malloc(sz);
    int offset = 0;

    char* segments[5] = {&byte1, &byte2, length, mask_key, f->data};
    int sizes[5] = {1, 1, len_sz, mask_key_sz, f->length};

    for (int i = 0; i < 5; i++) {
        if (segments[i] != NULL) {
            memcpy(*buf + offset, segments[i], sizes[i]);
            offset += sizes[i];
        }
    }

    if (length != NULL) {
        free(length);
    }
    
    if (mask_key != NULL) {
        free(mask_key);
    }

    return sz;
}

unsigned long ntohll(unsigned long n)
{
    unsigned long result = 0;
    int sz = sizeof(unsigned long);
    unsigned long mask = 0x00000000000000FF;

    for (int i = 0; i < sz; i++) {
        unsigned long tmp = n & mask;
        tmp = tmp >> i * 8;
        tmp = tmp << (sz * 7 - i * 8);
        result |= tmp;
        mask = mask << 8;
    }

    return result;
}