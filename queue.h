#ifndef SC6_QUEUE_H
#define SC6_QUEUE_H

#include "block.h"
#include "list.h"

void addQueueList(struct list *list, int threadNumber);

void deleteQueueList(struct list *list, int threadNumber);

void printQueueList(struct block *block, struct list *queueList);

#endif //SC6_QUEUE_H
