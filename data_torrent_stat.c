#include "data_torrent_stat.h"

#include <stdio.h>
#include "data_change.h"

#define DEBUG 0

/**
 * Меняет значение complete и incomplete торрента
 * @param torrent
 * @param oldEvent
 * @param newEvent
 * @param diff
 */
void torrentChangeStats(struct torrent *torrent, unsigned char oldEvent, unsigned char newEvent, char diff) {
    if (newEvent == EVENT_ID_COMPLETED) {
        DEBUG && printf("Downloaded set %d -> %d\n", torrent->downloaded, torrent->downloaded + 1);
        torrent->downloaded++;
    }

    if(newEvent == EVENT_ID_STOPPED) {
        if (oldEvent == EVENT_ID_COMPLETED) {
            torrent->complete--;
        } else {
            torrent->incomplete--;
        }

        return;
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
