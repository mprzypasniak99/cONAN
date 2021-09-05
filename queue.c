#include "queue.h"

void deleteQueue(queue* item) {
    if (queue.nextItem != NULL)
    {
        deleteQueue(queue.nextItem);
    }

    free(queue);
}