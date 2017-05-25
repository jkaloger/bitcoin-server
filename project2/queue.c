//
// Created by Jack on 24/5/17.
//

#include "queue.h"
#include <stdlib.h>
#include <string.h>

void enqueue(Queue *q, uint32_t diff, BYTE seed[64], uint64_t start, uint8_t worker_count)
{
    Queue new = malloc(sizeof(struct queue_t));
    Work w = malloc(sizeof(struct work_t));
    w->difficulty = diff;
    memcpy(w->seed, &seed, 64);
    w->start = start;
    w->worker_count = worker_count;
    new->work = w;
    new->next = NULL;

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
