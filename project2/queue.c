//
// Created by Jack on 24/5/17.
//

#include "queue.h"

void enqueue(Queue *q, uint32_t diff, BYTE seed[64], uint64_t start, uint8_t worker_count)
{
    Queue new = malloc(sizeof(struct queue_t));
    new->difficulty = diff;
    new->seed = seed;
    new->start = start;
    new->worker_count = worker_count;

    if(!*q) {
        *q = new;
    }
    else {
        Queue temp = *q;
        while(temp->next != NULL)
            temp = temp->next;
        temp->next = new;
    }
}

void dequeue(Queue *q)
{
    if(!*q)
        return;

    Queue toFree = *q;
    (*q) = (*q)->next;
    free(toFree);
}