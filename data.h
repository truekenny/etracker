#ifndef SC6_DATA_H
#define SC6_DATA_H

#include "uri.h"
#include "sem.h"
#include "block.h"
#include "data_structure.h"

struct torrent *deletePeer(struct firstByte *firstByte, struct query *query);
struct torrent *updatePeer(struct firstByte *firstByte, struct query *query);

#endif //SC6_DATA_H
