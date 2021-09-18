#ifndef QUEUE_H
#define QUEUE_H

typedef struct
{
    int destination;
    int priority;
    queue* nextItem;
} queue;

void deleteQueue(queue* item);

#endif