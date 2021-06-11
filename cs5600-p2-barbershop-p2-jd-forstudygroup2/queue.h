#ifndef __QUEUE_H__

#define __QUEUE_H__

#include <stdlib.h>

typedef struct queue_node_t
{
    void *data;
    struct queue_node_t *next;
} queue_node_t;

typedef struct queue_t
{
    queue_node_t* head;
    queue_node_t* tail; //reduces enqueue complexity to O(1)
    size_t size;
} queue_t; // default empty queue initialization

#define EMPTY_QUEUE_INITALIZER {NULL, 0}

int enqueue(queue_t *queue, void *element);

void* dequeue(queue_t *queue);

#endif // !__QUEUE_H__