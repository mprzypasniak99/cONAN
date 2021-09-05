#ifndef QUEUE_H
#define QUEUE_H

struct queue
{
    int destination;
    queue* nextItem;
};

void deleteQueue(queue* item);

#endif