//
// Created by Jack on 24/5/17.
//

#include "queue.h"
#include <stdlib.h>
#include <string.h>

void enqueue(Queue *q, int sockfd, char *diff_stream, char *seed_stream, char *start_stream, char *worker_stream)
{
    Queue new = malloc(sizeof(struct queue_t));
    new->sockfd = sockfd;
    Work w = malloc(sizeof(struct work_t));
    bzero(w->difficulty, 9);
    memcpy(w->difficulty, diff_stream, 8);
    bzero(w->seed, 65);
    memcpy(w->seed, seed_stream, 64);
    bzero(w->start, 17);
    memcpy(w->start, start_stream, 16);
    bzero(w->worker_count, 3);
    memcpy(w->worker_count, worker_stream, 2);
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

void removeWork(Queue *q, int sockfd)
{
    // the item was at the head of the list
    if((*q)->sockfd == sockfd) {
        Queue toFree = *q;
        *q = (*q)->next;
        free(toFree);
        return;
    }

    // iterator
    Queue temp = *q;
    while(temp->next != NULL) { // loop until end of list
        if((temp->next)->sockfd == sockfd) {
            // found our item
            temp->next = (temp->next)->next;
        }
        temp = temp->next;
    }
}