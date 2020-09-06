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
#include "string.h"

#define DEBUG 1
#define MSG_CONFIRM_ 0

void *clientUdpHandler(void *args) {
    int sockAddrSize = sizeof(struct sockaddr_in);
    struct clientUdpArgs *clientUdpArgs = (struct clientUdpArgs *) args;

    struct torrent *torrent = {0};
    struct block *block = initBlock();

    waitSem(clientUdpArgs->firstByte, clientUdpArgs->query);
    if (clientUdpArgs->query->event == EVENT_ID_STOPPED) {
        torrent = deletePeer(clientUdpArgs->firstByte, clientUdpArgs->query);
        DEBUG && printf("Delete UDP peer %s %s\n", clientUdpArgs->query->info_hash, clientUdpArgs->query->peer_id);
    } else {
        torrent = updatePeer(clientUdpArgs->firstByte, clientUdpArgs->query);
        DEBUG && printf("Update UDP peer %s %s\n", clientUdpArgs->query->info_hash, clientUdpArgs->query->peer_id);
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

    DEBUG && printHex(block->data, block->size);
    if (sendto(clientUdpArgs->serverSocket, block->data, block->size,
               MSG_CONFIRM_, (const struct sockaddr *) clientUdpArgs->clientAddr,
               sockAddrSize) == -1) {
        perror("Sendto failed");
        clientUdpArgs->stats->send_failed_udp++;
    } else {
        clientUdpArgs->stats->send_pass_udp++;
    }

    freeBlock(block);
    c_free(clientUdpArgs->clientAddr);
    c_free(clientUdpArgs->query);
    c_free(clientUdpArgs);

    if (pthread_detach(pthread_self()) != 0) {
        perror("Could not detach thread");
    }

    return 0;
}
