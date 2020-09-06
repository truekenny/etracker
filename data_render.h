#ifndef SC6_DATA_RENDER_H
#define SC6_DATA_RENDER_H

#include "block.h"
#include "data_structure.h"
#include "uri.h"

#define ENABLE_FULL_SCRAPE 0

void renderPeers(struct block *block, struct torrent *torrent, struct query *query);
void renderTorrents(struct block *block, struct firstByte *firstByte, struct block *hashes, _Bool udp);

#endif //SC6_DATA_RENDER_H
