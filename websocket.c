#include <memory.h>
#include <stdio.h>
#include "websocket.h"
#include "sha1.h"
#include "base64.h"
#include "socket.h"

#define WEBSOCKET_HEADER_KEY "Sec-WebSocket-Key: "
// GUID, RFC4122
#define WEBSOCKET_GUID "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"

#define WEBSOCKET_RETURN_CODE 13
#define WEBSOCKET_SPACE_CODE  32

#define WEBSOCKET_SIZE_FRAME 7

struct broadcastArgs {
    char *data;
    struct stats *stats;
};

struct block *websocketKey2Accept(char *data, int size) {
    char *headerKey = strstr(data, WEBSOCKET_HEADER_KEY);

    if (headerKey == NULL)
        return NULL;

    struct block *block = initBlock();

    for (long offset = headerKey - data + (int) strlen(WEBSOCKET_HEADER_KEY); offset < size - 1; ++offset) {
        char current = data[offset];

        if (current == WEBSOCKET_RETURN_CODE || current == WEBSOCKET_SPACE_CODE)
            break;

        addStringBlock(block, &current, 1);
    }

    if (!block->size) {
        freeBlock(block);

        return NULL;
    }

    addStringBlock(block, WEBSOCKET_GUID, strlen(WEBSOCKET_GUID));

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
    send_(socket, broadcastArgs->data, WEBSOCKET_SIZE_FRAME, broadcastArgs->stats);

    return LIST_CONTINUE_RETURN;
}

void broadcast(struct list *websockets, struct geoip *geoip, in_addr_t ip, struct stats *stats, unsigned char protocol) {
    if (ip == 0) {
        // unused
    }

    struct geoip *geoipSingle = findGeoip(geoip, htonl(ip));

    // 0x80 - FIN
    // 0x02 - BIN
    char data[WEBSOCKET_SIZE_FRAME] = "\x82\x05" "\x00\x00\x00\x00\x00";
    memcpy(data + 2, &geoipSingle->lat, 2);
    memcpy(data + 4, &geoipSingle->lon, 2);
    memcpy(data + 6, &protocol, 1);

    struct broadcastArgs broadcastArgs;
    broadcastArgs.stats = stats;
    broadcastArgs.data = data;

    mapList(websockets, &broadcastArgs, &broadcastCallback);
}
