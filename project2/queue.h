//
// Created by Jack on 24/5/17.
//

#ifndef QUEUE_H
#define QUEUE_H

#include <stdint.h>

#ifndef BYTE_TYPEDEF

#define BYTE_TYPEDEF

typedef unsigned char BYTE;

#endif
typedef struct queue_t *Queue;

struct queue_t {
    uint32_t difficulty;
    BYTE seed[64];
    uint64_t start;
    uint8_t worker_count;
    Queue next;
};

void enqueue(Queue *q, uint32_t diff, BYTE seed[64], uint64_t start, uint8_t worker_count);
void dequeue(Queue *q);

#endif //QUEUE_H
