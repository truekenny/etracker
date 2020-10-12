#include <memory.h>
#include <stdio.h>
#include "websocket.h"
#include "sha1.h"
#include "base64.h"
#include "socket.h"

#define HEADER_KEY "Sec-WebSocket-Key: "
// GUID, RFC4122
#define GUID_WEBSOCKET "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"
#define RETURN_CODE 13
#define SPACE_CODE 32
#define SIZE_FRAME 7

struct broadcastArgs {
    char *data;
    struct stats *stats;
};

struct block *websocketKey2Accept(char *data, int size) {
    char *headerKey = strstr(data, HEADER_KEY);

    if (headerKey == NULL)
        return NULL;

    struct block *block = initBlock();

    for (long offset = headerKey - data + (int) strlen(HEADER_KEY); offset < size - 1; ++offset) {
        char current = data[offset];

        if (current == RETURN_CODE || current == SPACE_CODE)
            break;

        addStringBlock(block, &current, 1);
    }

    if (!block->size) {
        freeBlock(block);

        return NULL;
    }

    addStringBlock(block, GUID_WEBSOCKET, strlen(GUID_WEBSOCKET));

    sha1(block);
    struct block *result = base64_encode(block);
    freeBlock(block);

    return result;
}

void deleteWebsocket(struct list *websockets, int socket) {
    waitSemaphoreLeaf(websockets);
    struct item *websocket = getHash(websockets, (unsigned char *) &socket);
    if (websocket != NULL) {
        deleteItem(websocket);
        // printf("websocket delete %d\n", socket);
    } else {
        // printf("not websocket %d\n", socket);
    }
    postSemaphoreLeaf(websockets);
}

unsigned char broadcastCallback(struct list *list, struct item *item, void *args) {
    if (list == 0) {
        // unused
    }
    struct broadcastArgs *broadcastArgs = args;

    int socket = *(int *) item->hash;
    send_(socket, broadcastArgs->data, SIZE_FRAME, broadcastArgs->stats);

    return RETURN_CONTINUE;
}

void broadcast(struct list *websockets, struct geoip *geoip, in_addr_t ip, struct stats *stats, unsigned char protocol) {
    if (ip == 0) {
        // unused
    }

    struct geoip *geoipSingle = findGeoip(geoip, htonl(ip));

    // 0x80 - FIN
    // 0x02 - BIN
    char data[SIZE_FRAME] = "\x82\x05" "\x00\x00\x00\x00\x00";
    memcpy(data + 2, &geoipSingle->lat, 2);
    memcpy(data + 4, &geoipSingle->lon, 2);
    memcpy(data + 6, &protocol, 1);

    struct broadcastArgs broadcastArgs;
    broadcastArgs.stats = stats;
    broadcastArgs.data = data;

    mapList(websockets, &broadcastArgs, &broadcastCallback);
}
