#include "list.h"

struct list_node* list_node_init(void* value)
{
    int sz = sizeof(struct list_node);
    struct list_node* node = (struct list_node*) malloc(sz);
    node->value = value;
    node->next = NULL;
    return node;
}

void list_free(struct list_node* list, void (*value_free)(void*))
{
    while (list != NULL) {
        struct list_node* tmp = list;
        list = list->next;
        value_free(tmp->value);
        free(tmp);
    }
}

int list_append(struct list_node* list, struct list_node* node)
{
    if (list == NULL || node == NULL) {
        return -1;
    }

    while (list->next != NULL) {
        list = list->next;
    }

    list->next = node;
    return 0;
}

int list_length(struct list_node* list)
{
    int length = 0;

    while (list != NULL) {
        length++;
        list = list->next;
    }

    return length;
}

void value_free_nofree(void* value)
{
    return;
}

void value_free_default(void* value)
{
    free(value);
}