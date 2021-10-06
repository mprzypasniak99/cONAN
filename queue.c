#include "queue.h"
#include "stdlib.h"

void deleteQueue(queue* item) {
    if (item->nextItem != NULL)
    {
        deleteQueue(item->nextItem);
    }

    free(item);
}

void addToQueue(queue** q, int dest, int priority) {
    queue* tmp = *q;

    queue* newElem = malloc(sizeof(queue));
    newElem->destination = dest;
    newElem->priority = priority;
    
    if (tmp == NULL || tmp->priority < priority) {
        newElem->nextItem = tmp;
        *q = newElem;
    } else {
        while(tmp != NULL) {
            if (tmp->nextItem == NULL) {
                newElem->nextItem = tmp->nextItem;
                tmp->nextItem = newElem;
                break;
            } else if (tmp->nextItem->priority < priority) {
                newElem->nextItem = NULL;
                tmp->nextItem = newElem;
            } else {
                tmp = tmp->nextItem;
            }
        }
    }
}

void deleteFromQueue(queue** q, int dest) {
    queue* tmp = *q;
    queue* del = NULL;

    if (tmp->destination == dest) {
        del = tmp;
        *q = tmp->nextItem;
    } else {
        while (tmp != NULL && tmp->nextItem != NULL) {
            if (tmp->nextItem->destination == dest) {
                del = tmp->nextItem;
                tmp->nextItem = del->nextItem;
                break;
            }
        }
    }

    if (del != NULL) {
        free(del);
    }
}