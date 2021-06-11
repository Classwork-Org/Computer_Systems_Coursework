#include <stdlib.h>
#include <stdio.h>
#include "queue.h"
#include <string.h>

struct process_t
{
    int id;
    char *name;
};

// prints contents of queue, node 0 is always head node
void print_process_queue(queue_t *queue)
{
    struct queue_node_t *cursor = queue->head;
    if (cursor == NULL)
    {
        printf("QUEUE IS EMPTY!\n");
    }
    else
    {
        printf("CURRENT PROCESS QUEUE CONTENTS:\n");
        unsigned int idx = 0;
        while (cursor != NULL)
        {
            struct process_t *process_under_cursor = ((struct process_t *)cursor->data);
            printf("NODE@%d: PROCESS_ID=%d, PROCESS_NAME=%s\n", idx++, process_under_cursor->id, process_under_cursor->name);
            cursor = cursor->next;
        }
    }
}

void print_process(struct process_t *proc)
{
    printf("PROCESS_ID=%d, PROCESS_NAME=%s\n", proc->id, proc->name);
}

void print_line(size_t size, char seperator)
{
    int idx;
    for(idx = 0; idx<size; idx++)
    {
        printf("%c", seperator);
    }
    printf("\n");
}

int run_enque_test_case(queue_t* queue, struct process_t* proc)
{
    printf("ENQUING PROCESS %s WITH ID %d\n", proc->name, proc->id);
    if (enqueue(queue, proc) == -1)
    {
        printf("FAILED TO ENQUEUE!...\n");
        return -1;
    }
    print_process_queue(queue);
    print_line(20, '*');
    return 0;
}

int run_dequeue_test_case(queue_t* queue, struct process_t* proc)
{
    struct process_t *dequeued_process;
    printf("DENQUING PROCESS %s WITH ID %d\n", proc->name, proc->id);
    if (!(dequeued_process = dequeue(queue)))
    {
        printf("FAILED TO DENQUEUE!...\n");
        return -1;
    }
    print_process_queue(queue);
    printf("RETRIEVED HEAD DATA: \n");
    print_process(dequeued_process);
    if((dequeued_process->id != proc->id) || (strcmp(dequeued_process->name, proc->name) != 0))
    {
        printf("DEQUEUED PROCESS DOES NOT MATCH EXPECTED PROCESS!...\n");
        return -1;
    }
    
    print_line(20, '*');
    return 0;
}

int main(int argc, char const *argv[])
{
    //relying on static data allocation to simplify leak checking. queue nodes
    //remain dynamically allocated, but data they point to is statically
    //allocated for demonstration simplicity
    struct process_t t1 = {1, "T1"};
    struct process_t t2 = {2, "T2"};
    struct process_t t3 = {3, "T3"};
    struct process_t t4 = {4, "T4"};
    queue_t queue = empty_queue;
    //-------------------------------BEGINING ENQUEUE--------------------------------
    print_line(20, '*');
    print_process_queue(&queue);

    //-------------------------------FIRST ENQUEUE--------------------------------
    if(run_enque_test_case(&queue, &t1))
    {
        printf("FIRST ENQUEUE FAILED!\n");
    }

    //-------------------------------SECOND ENQUEUE--------------------------------
    if(run_enque_test_case(&queue, &t2))
    {
        printf("SECOND ENQUEUE FAILED!\n");
    }

    //-------------------------------THIRD ENQUEUE--------------------------------
    if(run_enque_test_case(&queue, &t3))
    {
        printf("THIRD ENQUEUE FAILED!\n");
    }

    //-------------------------------FOURTH ENQUEUE--------------------------------
    if(run_enque_test_case(&queue, &t4))
    {
        printf("FOURTH ENQUEUE FAILED!\n");
    }


    //-------------------------------BEGINING DEQUEUE--------------------------------
    print_line(50, '-');
    print_process_queue(&queue);

    //-------------------------------FIRST DEQUEUE--------------------------------
    if(run_dequeue_test_case(&queue, &t1))
    {
        printf("FIRST DEQUEUE FAILED!\n");
    }

    //-------------------------------SECOND DEQUEUE--------------------------------
    if(run_dequeue_test_case(&queue, &t2))
    {
        printf("SECOND DEQUEUE FAILED!\n");
    }

    //-------------------------------THIRD DEQUEUE--------------------------------
    if(run_dequeue_test_case(&queue, &t3))
    {
        printf("THIRD DEQUEUE FAILED!\n");
    }

    //-------------------------------FOURTH DEQUEUE--------------------------------
    if(run_dequeue_test_case(&queue, &t4))
    {
        printf("FOURTH DEQUEUE FAILED!\n");
    }

    return 0;
}
