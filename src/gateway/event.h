#ifndef GATEWAY_EVENT_H
#define GATEWAY_EVENT_H

struct event {
    int opcode;
    int seq;
    char* name;
    char* data;
};

#endif