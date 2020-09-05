#include <pthread.h>
#include <stdio.h>
#include "thread_client_udp.h"
#include "alloc.h"
#include "uri.h"
#include "data_sem.h"
#include "data_structure.h"
#include "data_change.h"
#include "data_render.h"
#include "socket_udp_response_structure.h"
#include "stats.h"

#define MSG_CONFIRM 0

void *clientUdpHandler(void *args) {
    int sockAddrSize = sizeof(struct sockaddr_in);
    struct clientUdpArgs *clientUdpArgs = (struct clientUdpArgs *) args;

    struct torrent *torrent = {0};
    struct block *block = initBlock();

    waitSem(clientUdpArgs->firstByte, clientUdpArgs->query);
    if (clientUdpArgs->query->event == EVENT_ID_STOPPED) {
        torrent = deletePeer(clientUdpArgs->firstByte, clientUdpArgs->query);
    } else {
        torrent = updatePeer(clientUdpArgs->firstByte, clientUdpArgs->query);
    }

    struct announceHeadResponse announceHeadResponse = {0};
    announceHeadResponse.action = ntohl(ACTION_ANNOUNCE);
    announceHeadResponse.transaction_id = clientUdpArgs->transaction_id;
    announceHeadResponse.interval = ntohl(INTERVAL);
    if (torrent != NULL) {
        announceHeadResponse.leechers = ntohl(torrent->incomplete);
        announceHeadResponse.seeders = ntohl(torrent->complete);
    }
    addStringBlock(block, &announceHeadResponse, sizeof(struct announceHeadResponse));

    renderPeers(block, torrent, clientUdpArgs->query);
    postSem(clientUdpArgs->firstByte, clientUdpArgs->query);

    clientUdpArgs->stats->sent_bytes_udp += block->size;
    if (sendto(clientUdpArgs->serverSocket, block->data, block->size,
               MSG_CONFIRM, (const struct sockaddr *) clientUdpArgs->clientAddr,
               sockAddrSize) == -1) {
        perror("Sendto failed");
        clientUdpArgs->stats->send_failed_udp++;
    } else {
        clientUdpArgs->stats->send_pass_udp++;
    }

    c_free(block);
    c_free(clientUdpArgs->clientAddr);
    c_free(clientUdpArgs->query);
    c_free(clientUdpArgs);

    if (pthread_detach(pthread_self()) != 0) {
        perror("Could not detach thread");
    }

    return 0;
}
