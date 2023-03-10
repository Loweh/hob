#ifndef GATEWAY_READY_DATA_H
#define GATEWAY_READY_DATA_H
#define JSMN_HEADER

#include <stdlib.h>
#include <string.h>

#include "app_data.h"
#include "../ext/jsmn.h"

struct ready_data {
    char* app_id;
    char* session_id;
    char* resume_url;
};

int get_ready_data(char* buf, struct ready_data* d);

#endif