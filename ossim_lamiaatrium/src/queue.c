#include <stdio.h>
#include <stdlib.h>
#include "queue.h"

int empty(struct queue_t *q)
{
        if (q == NULL)
                return 1;
        return (q->size == 0);
}

void enqueue(struct queue_t *q, struct pcb_t *proc)
{
    if (!q || !proc) return;

    if (q->size >= MAX_QUEUE_SIZE) {
        printf("Queue limit exceeded!\n");
        return;
    }

    //update assign proc, update tail and size
    q->proc[q->tail] = proc;
    q->tail = (q->tail + 1) % MAX_QUEUE_SIZE;
    q->size++;
}

struct pcb_t *dequeue(struct queue_t *q)
{
        /* TODO: return a pcb whose priority is the highest
         * in the queue [q] and remember to remove it from q
         * */
        if (!q || q->size == 0) return NULL;

        //extract process and clear
        struct pcb_t *p_out = q->proc[q->head]; 
        q->proc[q->head] = NULL; 

        //update head and size
        q->head = (q->head + 1) % MAX_QUEUE_SIZE;
        q->size --;

        return p_out;
}

struct pcb_t *purgequeue(struct queue_t *q, struct pcb_t *proc)
{
        /* TODO: remove a specific item from queue
         * */

        if(!q || !proc ||q->size == 0) return NULL;

        // iterate through the queue, take the desired proccess
        int curr = q->head;
        int proc_idx = -1;
        for (int i = 0; i < q->size; i++)
        {
                if(q->proc[curr] == proc)
                {
                        proc_idx = curr;
                        break;
                }
                curr = (curr + 1) % MAX_QUEUE_SIZE;

        }
        // if there is no match
        if(proc_idx == -1) return NULL;

        //shift elements to the left
        int next = (curr + 1) % MAX_QUEUE_SIZE;
        while(next != q->tail)
        {
                q->proc[curr] = q->proc[next];
                curr = next;
                next = (next + 1) % MAX_QUEUE_SIZE;
        }

        // update tail, size
        q->proc[q->tail] = NULL;
        q->tail = (q->tail - 1 + MAX_QUEUE_SIZE) % MAX_QUEUE_SIZE;
        q->size --;
        return proc;
}