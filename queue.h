#ifndef QUEUE_H
#define QUEUE_H

typedef struct queue
{
    int destination;
    int priority;
    struct queue* nextItem;
} queue;

typedef struct errand {
    int errandNum;
    int available;
    int ack_destination;
    int priority;
} errand;

void deleteQueue(queue* item);

void addToQueue(queue** q, int dest, int priority);

void deleteFromQueue(queue** q, int dest);

#endif