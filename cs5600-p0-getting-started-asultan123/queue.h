#ifndef __QUEUE_H__

#define __QUEUE_H__

#include <stdlib.h>
#include <stdio.h>

struct queue_node_t
{
    void *data;
    struct queue_node_t *next;
};

struct queue_t
{
    struct queue_node_t* head;
    struct queue_node_t* tail; //reduces enqueue complexity to O(1)
    size_t size;
} empty_queue = {NULL, 0}; // default empty queue initialization

typedef struct queue_t queue_t;

int enqueue(queue_t *queue, void *element)
{
    //if queue is empty, create head node and set data to element pointer
    if(queue->size == 0)
    {
        if (!(queue->head = (struct queue_node_t *)malloc(sizeof(struct queue_node_t))))
        {
            return -1;
        }
        queue->head->data = element;
        queue->head->next = NULL;
        queue->tail = NULL;
        queue->size++;
    }
    //if queue already has a head, instantiate tail and point head to it
    else if(queue->size == 1)
    {
        if (!(queue->tail = (struct queue_node_t *)malloc(sizeof(struct queue_node_t))))
        {
            return -1;
        }
        queue->tail->data = element;
        queue->head->next = queue->tail;
        queue->size++;
    }
    //use tail to append new nodes
    else
    {
        if (!(queue->tail->next = (struct queue_node_t *)malloc(sizeof(struct queue_node_t))))
        {
            return -1;
        }
        queue->tail = queue->tail->next;
        queue->tail->data = element;
        queue->tail->next = NULL;
    }
    return 0;
}

void* dequeue(queue_t *queue)
{
    // if queue is not empty, return head data pointer, free head node and move
    // queue head pointer to next node in the queue
    if(queue->size>0)
    {
        struct queue_node_t* head = queue->head;
        void* data = head->data;
        queue->head = head->next;
        free(head);
        return data;
    }
    else
    {
        return NULL;
    }
    
}

#endif // !__QUEUE_H__