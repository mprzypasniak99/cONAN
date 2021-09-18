#ifndef QUEUE_H
#define QUEUE_H

typedef struct
{
    int destination;
    queue* nextItem;
} queue;

void deleteQueue(queue* item);

#endif