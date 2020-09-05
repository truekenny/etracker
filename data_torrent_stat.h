#ifndef SC6_DATA_TORRENT_STAT_H
#define SC6_DATA_TORRENT_STAT_H

#include "data_structure.h"

void torrentChangeStats(struct torrent *torrent, unsigned char oldEvent, unsigned char newEvent, char diff);

#endif //SC6_DATA_TORRENT_STAT_H
