#include "queue.h"
#include "stdlib.h"

void deleteQueue(queue* item) {
    if (item->nextItem != NULL)
    {
        deleteQueue(item->nextItem);
    }

    free(item);
}