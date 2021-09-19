#ifndef QUEUE_H
#define QUEUE_H

typedef struct queue
{
    int destination;
    int priority;
    struct queue* nextItem;
} queue;

void deleteQueue(queue* item);

#endif