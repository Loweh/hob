#include "ws_frame.h"

char* reverse_bytes(char* src, int n)
{
    char* reverse = (char*) malloc(n);

    for (int i = 0; i < n; i++) {
        reverse[i] = src[n - 1 - i];
    }

    return reverse;
}



// Add length error checking on n, where n is the length of raw
// Refactor so it looks better
struct ws_frame* ws_deserialize_frame(char* raw, int n)
{
    struct ws_frame* frame = (struct ws_frame*) malloc(sizeof(struct ws_frame));
    int offset = 0;
    // First, get the fin and opcode bits from the first byte
    frame->fin = (raw[offset] & 0b10000000) >> 7;
    frame->opcode = raw[offset] & 0b00001111;
    offset++;
    // Then, get the mask and initial length bits from the second byte
    frame->mask = (raw[offset] & 0b10000000) >> 7;
    int init_len = raw[offset] & 0b01111111;
    offset++;

    if (init_len < 126) {
        frame->length = init_len;
    } else if (init_len == 126) {
        // Frame length is 126, so the next 2 bytes are the length
        char* reverse = reverse_bytes(raw + offset, 2);
        memcpy(&frame->length, reverse, 2);
        free(reverse);
        offset += 2;
    } else {
        // Frame length is 127, so the next 8 bytes are the length
        char* reverse = reverse_bytes(raw + offset, 8);
        memcpy(&frame->length, reverse, 8);
        free(reverse);
        offset += 8;
    }

    if (frame->mask) {
        // Mask is enabled, so the next 4 bytes will be the mask key
        char* reverse = reverse_bytes(raw + offset, 4);
        memcpy(&frame->mask_key, reverse, 4);
        free(reverse);
        offset += 4;
    }

    frame->payload = (char*) malloc(frame->length);
    memcpy(frame->payload, raw + offset, frame->length);

    return frame;
}

int ws_serialize_frame(struct ws_frame* frame, char** data)
{
    int sz = 2; // A websocket message must consist of at least two bytes

    sz += frame->length;

    if (frame->length > 125) {
        if (frame->length <= 0xFFFF) {
            sz += 2; // Frame length fits in 2 bytes, so add 2 bytes for length
        } else {
            sz += 8; // Frame length fits in 8 bytes, so add 8 bytes for length
        }
    }

    if (frame->mask) {
        sz += 4; // Mask key needs 4 bytes
    }

    *data = (char*) malloc(sz);
    int offset = 0;

    char byte1 = frame->fin;
    byte1 = byte1 << 7; // Set fin bit to MSB
    byte1 |= frame->opcode; // Set opcode as the 4 LSBs
    memcpy(*data + offset, &byte1, 1);
    offset++;

    int next_bytes = 0;
    char byte2 = frame->mask;
    byte2 = byte2 << 7; // Set mask bit to MSB

    if (frame->length < 126) {
        byte2 |= frame->length;
    } else if (frame->length <= 0xFFFF) {
        byte2 |= 126;
        next_bytes = 2;
    } else {
        byte2 |= 127;
        next_bytes = 8;
    }

    memcpy(*data + offset, &byte2, 1);
    offset++;

    if (next_bytes != 0) {
        // First convert the int to char*
        char* original = (char*) malloc(next_bytes);
        memcpy(original, &frame->length, next_bytes);
        // Then reverse the byte placement and copy it into the buffer
        char* reverse = reverse_bytes(original, next_bytes);
        memcpy(*data + offset, reverse, next_bytes);

        free(original);
        free(reverse);
        offset += next_bytes;
    }

    if (frame->mask) {
        char original[4] = {0};
        memcpy(original, &frame->mask_key, 4);
        char* reverse = reverse_bytes(original, 4);
        memcpy(*data + offset, reverse, 4);

        free(reverse);
        offset += 4;
    }

    memcpy(*data + offset, frame->payload, frame->length);
    offset += frame->length;

    return offset;
}

void ws_free_frame(struct ws_frame** frame)
{
    struct ws_frame*f = *frame;
    free(f->payload);
    free(f);
    *frame = NULL;
}