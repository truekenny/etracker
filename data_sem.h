#ifndef SC6_DATA_SEM_H
#define SC6_DATA_SEM_H

#include "data_structure.h"
#include "uri.h"

void initSem(struct firstByte *firstByte);
void waitSem(struct firstByte *firstByte, struct query *query);
void postSem(struct firstByte *firstByte, struct query *query);


#endif //SC6_DATA_SEM_H
