#ifndef SC6_DATA_H
#define SC6_DATA_H

#include "list.h"
#include "uri.h"
#include "interval.h"
#include "stats.h"

#define DATA_FULL_SCRAPE_ENABLE 0

void renderAnnouncePublic(struct block *block, struct block *announceBlock, struct item *torrent, struct query *query, struct interval *interval);

void renderScrapeTorrentsPublic(struct block *block, struct block *scrapeBlock, struct list *torrentList, struct block *hashes, struct query *query);

struct item *deletePeerPublic(struct list *torrentList, struct query *query);

struct item *setPeerPublic(struct list *torrentList, struct query *query, unsigned char protocol, struct stats *stats);

#endif //SC6_DATA_H
