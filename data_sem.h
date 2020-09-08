#ifndef SC6_DATA_SEM_H
#define SC6_DATA_SEM_H

#include "data_structure.h"
#include "uri.h"

void initSem(struct firstByteData *firstByte);
void waitSem(struct firstByteData *firstByte, struct query *query);
void postSem(struct firstByteData *firstByte, struct query *query);


#endif //SC6_DATA_SEM_H
