#ifndef UTIL_LIST_H
#define UTIL_LIST_H

#include <stdlib.h>

struct list_node {
    void* value;
    void* next;
};

struct list_node* list_node_init(void* value);
void list_free(struct list_node* list, void (*value_free)(void*));

int list_append(struct list_node* list, struct list_node* node);
int list_length(struct list_node* list);

void value_free_nofree(void* value);
void value_free_default(void* value);

#endif