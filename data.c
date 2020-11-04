#include <time.h>
#include <memory.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include "data.h"
#include "alloc.h"
#include "socket_udp_structure.h"
#include "data_structure.h"
#include "socket.h"

#define DATA_SEPARATE_TCP_AND_UDP_CLIENTS 0
/*
 * Приблизительно максимальный размер сообщения анонса и скрапа
 * Максимально допустимые размер UDP данных 65 527 байт
 */
#define DATA_MAX_PACKET_SIZE 64000

struct renderAnnouncePeersCallbackL {
    struct block *block;
    unsigned int leftPeers;
    struct query *query;
};

struct renderScrapeTorrentsCallbackL {
    struct block *block;
    struct query *query;
};

void int2ip(char *dest, struct in6_addr *source, unsigned char ipVersion);

void changeTorrentStatsL(struct item *torrent, unsigned char oldEvent, unsigned char newEvent);

struct item *setTorrentL(struct list *torrentList, unsigned char *hash) {
    struct item *torrent = setHash(torrentList, hash);


    if (torrent->data == NULL) {
        torrent->data = c_calloc(1, sizeof(struct torrentDataL));
        ((struct torrentDataL *) torrent->data)->peerList =
                initList(NULL, 0, LIST_STARTING_NEST, URI_PARAM_VALUE_LENGTH, LIST_SEMAPHORE_DISABLE, BIG_ENDIAN);
    }

    return torrent;
}

/**
 * Рендер отдельного пира для анонса
 * @param list
 * @param peer
 * @param args
 * @return
 */
unsigned char renderAnnouncePeersCallback(struct list *list, struct item *peer, void *args) {
    if (list == NULL) {
        // error: unused parameter 'list' [-Werror,-Wunused-parameter]
    }

    struct renderAnnouncePeersCallbackL *mapPeersL = args;
    struct query *query = mapPeersL->query;
    struct block *peerBlock = mapPeersL->block;
    struct peerDataL *peerDataL = peer->data;

    if (peerBlock->size >= DATA_MAX_PACKET_SIZE) {

        return LIST_BREAK_RETURN;
    }

    if (DATA_SEPARATE_TCP_AND_UDP_CLIENTS) {
        // Ищу TCP, но пир не TCP
        if (query->protocol == URI_QUERY_PROTOCOL_TCP
            && (peerDataL->protocol & DATA_STRUCTURE_PEER_PROTOCOL_TCP_BIT) == 0) {

            return LIST_CONTINUE_RETURN;
        }
        // Ищу UDP, но пир не UDP
        if (query->protocol == URI_QUERY_PROTOCOL_UDP
            && (peerDataL->protocol & DATA_STRUCTURE_PEER_PROTOCOL_UDP_BIT) == 0) {

            return LIST_CONTINUE_RETURN;
        }
    }

    // Версии протоколов разные (ipv4/ipv6)
    if ((query->ipVersion & peerDataL->ipVersion) == 0) {

        return LIST_CONTINUE_RETURN;
    }

    char ip[INET6_ADDRSTRLEN] = {0};

    if (query->compact || query->protocol == URI_QUERY_PROTOCOL_UDP) {
        // todo сделать правильный разбор ip4/6
        if (query->ipVersion & SOCKET_VERSION_IPV4_BIT) {
            unsigned char *ip6 = (unsigned char *) &peerDataL->ip4;
            unsigned char *ip4 = ip6 + 12;
            addStringBlock(peerBlock, ip4, sizeof(int));
        } else
            addStringBlock(peerBlock, &peerDataL->ip6, sizeof(struct in6_addr));

        addStringBlock(peerBlock, &peerDataL->port, sizeof(short));
    } else if (query->no_peer_id) {
        // todo сделать правильный разбор ip4/6
        if (query->ipVersion & SOCKET_VERSION_IPV4_BIT)
            int2ip(ip, &peerDataL->ip4, query->ipVersion);
        else
            int2ip(ip, &peerDataL->ip6, query->ipVersion);
        addFormatStringBlock(peerBlock, 500,
                             "d"
                             "4:port" "i%de"
                             "2:ip" "%lu:%s"
                             "e",
                             htons(peerDataL->port),
                             strlen(ip),
                             ip
        );
    } else {
        addFormatStringBlock(peerBlock, 500,
                             "d"
                             "4:port" "i%de"
                             "7:peer id" "%d:",
                             htons(peerDataL->port),
                             URI_PARAM_VALUE_LENGTH
        );

        addStringBlock(peerBlock, peer->hash, URI_PARAM_VALUE_LENGTH);

        // todo сделать правильный разбор ip4/6
        if (query->ipVersion & SOCKET_VERSION_IPV4_BIT)
            int2ip(ip, &peerDataL->ip4, query->ipVersion);
        else
            int2ip(ip, &peerDataL->ip6, query->ipVersion);
        addFormatStringBlock(peerBlock, 500,
                             "2:ip" "%lu:%s"
                             "e",
                             strlen(ip),
                             ip
        );
    }

    return --mapPeersL->leftPeers <= 0;
}

/**
 * Рендер пиров торрента для анонса
 * @param block
 * @param peerList
 * @param query
 */
void renderAnnouncePeersL(struct block *block, struct list *peerList, struct query *query) {
    struct renderAnnouncePeersCallbackL mapPeersL;
    mapPeersL.block = block;
    mapPeersL.leftPeers = query->numwant; // Изменяемое значение
    mapPeersL.query = query;

    mapList(peerList, &mapPeersL, &renderAnnouncePeersCallback);
}

/**
 * Рендер заголовка анонса
 * @param block
 * @param torrent
 * @param query
 * @param interval
 */
void renderAnnounceTorrentL(struct block *block, struct block *announceBlock, struct item *torrent, struct query *query,
                            struct interval *interval) {
    struct torrentDataL torrentDataL = {};
    if (torrent != NULL)
        memcpy(&torrentDataL, torrent->data, sizeof(struct torrentDataL));

    struct list *peerList = torrentDataL.peerList;
    announceBlock = resetBlock(announceBlock);
    renderAnnouncePeersL(announceBlock, peerList, query);

    char *peersByVersion = (query->ipVersion & SOCKET_VERSION_IPV4_BIT) ? "5:peers" : "6:peers6";

    if (query->protocol == URI_QUERY_PROTOCOL_UDP) {}
    else if (query->compact) {
        addFormatStringBlock(block, 500,
                             "d"
                             "8:complete" "i%de"
                             "10:incomplete" "i%de"
                             "10:downloaded" "i%de"
                             "8:interval" "i%de"
                             "%s"
                             "%d:",
                             torrentDataL.complete,
                             torrentDataL.incomplete,
                             torrentDataL.downloaded,
                             interval->interval,
                             peersByVersion,
                             announceBlock->size
        );
    } else {
        addFormatStringBlock(block, 500,
                             "d"
                             "8:complete" "i%de"
                             "10:incomplete" "i%de"
                             "10:downloaded" "i%de"
                             "8:interval" "i%de"
                             "%s"
                             "l",
                             torrentDataL.complete,
                             torrentDataL.incomplete,
                             torrentDataL.downloaded,
                             interval->interval,
                             peersByVersion
        );
    }

    addStringBlock(block, announceBlock->data, announceBlock->size);

    if (query->protocol == URI_QUERY_PROTOCOL_TCP) {
        if (!query->compact) {
            addStringBlock(block, "e", sizeof(char));
        }

        addStringBlock(block, "e", sizeof(char));
    }
}

/**
 * Рендер анонса
 * @param block
 * @param torrent
 * @param query
 * @param interval
 */
void renderAnnouncePublic(struct block *block, struct block *announceBlock, struct item *torrent, struct query *query,
                          struct interval *interval) {
    struct torrentDataL torrentDataL = {};
    if (torrent != NULL)
        memcpy(&torrentDataL, torrent->data, sizeof(struct torrentDataL));

    if (query->protocol == URI_QUERY_PROTOCOL_UDP) {
        struct announceHeadResponse announceHeadResponse = {};
        announceHeadResponse.action = ntohl(SOCKET_UDP_STRUCTURE_ACTION_ANNOUNCE);
        announceHeadResponse.transaction_id = query->transaction_id;
        announceHeadResponse.interval = ntohl(interval->interval);

        announceHeadResponse.leechers = ntohl(torrentDataL.incomplete);
        announceHeadResponse.seeders = ntohl(torrentDataL.complete);

        addStringBlock(block, &announceHeadResponse, sizeof(struct announceHeadResponse));

    }

    renderAnnounceTorrentL(block, announceBlock, torrent, query, interval);
}

/**
 * Рендер статитики пиров одного торрента для скрапа
 * @param block
 * @param torrent
 * @param hash
 * @param protocol
 */
void renderScrapeTorrentL(struct block *block, struct item *torrent, unsigned char *hash, _Bool protocol) {
    struct torrentDataL torrentDataL = {};
    if (torrent != NULL)
        memcpy(&torrentDataL, torrent->data, sizeof(struct torrentDataL));

    if (protocol == URI_QUERY_PROTOCOL_UDP) {
        struct scrapeTorrentResponse scrapeTorrentResponse = {};

        scrapeTorrentResponse.seeders = htonl(torrentDataL.complete);
        scrapeTorrentResponse.completed = htonl(torrentDataL.downloaded);
        scrapeTorrentResponse.leechers = htonl(torrentDataL.incomplete);

        addStringBlock(block, &scrapeTorrentResponse, sizeof(struct scrapeTorrentResponse));
    } else {
        addStringBlock(block, "20:", 3);

        addStringBlock(block, hash, URI_PARAM_VALUE_LENGTH);

        addFormatStringBlock(block, 1000, "d"
                                          "8:completei%de"
                                          "10:incompletei%de"
                                          "10:downloadedi%de"
                                          "e",
                             torrentDataL.complete,
                             torrentDataL.incomplete,
                             torrentDataL.downloaded);
    }
}

/**
 * Перебор всех торрентов для скрапа
 * @param list
 * @param torrent
 * @param args
 * @return
 */
unsigned char renderScrapeTorrentsCallback(struct list *list, struct item *torrent, void *args) {
    if (list == NULL) {
        // error: unused parameter 'list' [-Werror,-Wunused-parameter]
    }

    struct renderScrapeTorrentsCallbackL *renderScrapeTorrentsCallbackL = args;

    renderScrapeTorrentL(renderScrapeTorrentsCallbackL->block, torrent, torrent->hash,
                         renderScrapeTorrentsCallbackL->query->protocol);

    return LIST_CONTINUE_RETURN;
}

/**
 * Рендер скрапа
 * @param block
 * @param torrentList
 * @param hashes
 * @param query
 */
void
renderScrapeTorrentsPublic(struct block *block, struct block *scrapeBlock, struct list *torrentList,
                           struct block *hashes, struct query *query) {
    scrapeBlock = resetBlock(scrapeBlock);

    _Bool protocol = query->protocol;

    if (protocol == URI_QUERY_PROTOCOL_UDP) {
        struct scrapeHeadResponse scrapeHeadResponse = {};
        scrapeHeadResponse.action = htonl(SOCKET_UDP_STRUCTURE_ACTION_SCRAPE);
        scrapeHeadResponse.transaction_id = query->transaction_id;
        addStringBlock(block, &scrapeHeadResponse, sizeof(struct scrapeHeadResponse));

    } else {
        addStringBlock(block, "d"
                              "5:files"
                              "d", 9);
    }

    if (hashes->size == 0 && DATA_FULL_SCRAPE_ENABLE) {
        struct renderScrapeTorrentsCallbackL renderScrapeTorrentsCallbackL = {};
        renderScrapeTorrentsCallbackL.query = query;
        renderScrapeTorrentsCallbackL.block = scrapeBlock;
        mapList(torrentList, &renderScrapeTorrentsCallbackL, &renderScrapeTorrentsCallback);

    } else {
        unsigned char hash[URI_PARAM_VALUE_LENGTH];

        for (unsigned int index = 0; index < hashes->size / URI_PARAM_VALUE_LENGTH; ++index) {
            memcpy(&hash, &hashes->data[index * URI_PARAM_VALUE_LENGTH], URI_PARAM_VALUE_LENGTH);

            struct list *leaf = getLeaf(torrentList, hash);
            waitSemaphoreLeaf(leaf);

            struct item *torrent = getHash(torrentList, hash);
            renderScrapeTorrentL(scrapeBlock, torrent, hash, protocol);

            postSemaphoreLeaf(leaf);

            if (scrapeBlock->size >= DATA_MAX_PACKET_SIZE) {
                break;
            }
        }
    }

    addStringBlock(block, scrapeBlock->data, scrapeBlock->size);

    if (protocol == URI_QUERY_PROTOCOL_TCP) {
        addStringBlock(block, "e"
                              "e", 2);
    }
}

/**
 * Удаляет пира из торрента
 * @param torrentList
 * @param query
 * @return
 */
struct item *deletePeerPublic(struct list *torrentList, struct query *query) {
    struct item *torrent = getHash(torrentList, query->info_hash);

    if (torrent != NULL) {
        struct list *peerList = ((struct torrentDataL *) torrent->data)->peerList;

        struct item *peer = getHash(peerList, query->peer_id);

        if (peer != NULL) {
            struct peerDataL *peerDataL = peer->data;

            // Удаление из статистики
            changeTorrentStatsL(torrent, peerDataL->event, URI_EVENT_ID_NONE);

            deleteItem(peer);
        }
    }

    return torrent;
}

/**
 * Обновляет пира в торренте
 * @param torrentList
 * @param query
 * @return
 */
struct item *setPeerPublic(struct list *torrentList, struct query *query, unsigned char protocol, struct stats *stats) {
    struct item *torrent = setTorrentL(torrentList, query->info_hash);
    struct torrentDataL *torrentDataL = torrent->data;

    struct list *peerList = torrentDataL->peerList;

    if (torrentDataL->complete + torrentDataL->incomplete > DATA_STRUCTURE_PEERS_FOR_LEVEL_0_LIMIT
        && peerList->level == 0) {
        peerList = reInitList(peerList, 1);
        torrentDataL->peerList = peerList;
    }

    struct item *peer = setHash(peerList, query->peer_id);

    if (peer->data == NULL) {
        peer->data = c_calloc(1, sizeof(struct peerDataL));

        // Новый пир
        changeTorrentStatsL(torrent, URI_EVENT_ID_NONE, query->event);
    } else {

        // Уже имеющийся пир
        changeTorrentStatsL(torrent, ((struct peerDataL *) peer->data)->event, query->event);
    }

    struct peerDataL *peerData = (struct peerDataL *) peer->data;

    peerData->port = query->port;
    if (query->ipVersion & SOCKET_VERSION_IPV4_BIT)
        peerData->ip4 = query->ip;
    else
        peerData->ip6 = query->ip;

    time_t now = time(NULL);
    unsigned int delay = 0;
    if (peerData->updateTime != 0)
        delay = now - peerData->updateTime;
    updatePeerStat(stats, delay);

    peerData->updateTime = now;
    peerData->event = query->event;
    peerData->protocol = peerData->protocol | protocol;
    peerData->ipVersion = peerData->ipVersion | query->ipVersion;

    return torrent;
}

/**
 * Обновляет статистику пировов торрента
 * @param torrent
 * @param oldEvent
 * @param newEvent
 */
void changeTorrentStatsL(struct item *torrent, unsigned char oldEvent, unsigned char newEvent) {
    struct torrentDataL *torrentDataL = torrent->data;

    if (oldEvent == URI_EVENT_ID_NONE) {
        // Новый пир
        if (newEvent == URI_EVENT_ID_COMPLETED) {
            torrentDataL->downloaded++;
            torrentDataL->complete++;
        } else {
            torrentDataL->incomplete++;
        }
    } else if (newEvent == URI_EVENT_ID_NONE) {
        // Удаляем пир (пир есть в базе, он был добавлен, он действительно удаляется)
        if (oldEvent == URI_EVENT_ID_COMPLETED) {
            torrentDataL->complete--;
        } else {
            torrentDataL->incomplete--;
        }
    } else if (oldEvent != newEvent && newEvent != URI_EVENT_ID_CONTINUE) {
        // Пир меняет статус
        if (oldEvent == URI_EVENT_ID_COMPLETED) {
            torrentDataL->complete--;
            torrentDataL->incomplete++;
        } else if (newEvent == URI_EVENT_ID_COMPLETED) {
            torrentDataL->downloaded++;
            torrentDataL->complete++;
            torrentDataL->incomplete--;
        }
    }
}

// todo сделать правильный разбор ip4/6
void int2ip(char *dest, struct in6_addr *source, unsigned char ipVersion) {
    unsigned char *src6 = (unsigned char *) source;
    unsigned char *src4 = src6 + 12;

    memset(dest, 66, INET6_ADDRSTRLEN);

    if (ipVersion & SOCKET_VERSION_IPV4_BIT) {
        if (inet_ntop(AF_INET, src4, dest, INET_ADDRSTRLEN)) {

        }
    } else {
        if (inet_ntop(AF_INET6, source, dest, INET6_ADDRSTRLEN)) {

        }
    }
}
