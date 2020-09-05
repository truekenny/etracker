#include "data_torrent_stat.h"

#include "data_change.h"

/**
 * Меняет значение complete и incomplete торрента
 * @param torrent
 * @param oldEvent
 * @param newEvent
 * @param diff
 */
void torrentChangeStats(struct torrent *torrent, unsigned char oldEvent, unsigned char newEvent, char diff) {
    if (newEvent == EVENT_ID_COMPLETED) {
        torrent->downloaded++;
    }

    if (oldEvent == EVENT_ID_COMPLETED && newEvent == EVENT_ID_COMPLETED) {
        torrent->complete += diff;
    } else if (oldEvent != EVENT_ID_COMPLETED && newEvent != EVENT_ID_COMPLETED) {
        torrent->incomplete += diff;
    } else if (oldEvent != EVENT_ID_COMPLETED && newEvent == EVENT_ID_COMPLETED) {
        torrent->incomplete -= diff;
        torrent->complete += diff;
    } else if (oldEvent == EVENT_ID_COMPLETED && newEvent != EVENT_ID_COMPLETED) {
        torrent->incomplete += diff;
        torrent->complete -= diff;
    }
}
