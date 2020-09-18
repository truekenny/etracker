#ifndef SC6_DATA_H
#define SC6_DATA_H

#include "list.h"
#include "uri.h"

#define ENABLE_FULL_SCRAPE 0

void renderAnnouncePublic(struct block *block, struct item *torrent, struct query *query, unsigned int interval);

void renderScrapeTorrentsPublic(struct block *block, struct list *torrentList, struct block *hashes, struct query *query);

struct item *deletePeerPublic(struct list *torrentList, struct query *query);

struct item *setPeerPublic(struct list *torrentList, struct query *query);

#endif //SC6_DATA_H
