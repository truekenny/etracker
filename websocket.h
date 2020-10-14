#ifndef SC6_WEBSOCKET_H
#define SC6_WEBSOCKET_H

#include <arpa/inet.h>
#include "block.h"
#include "list.h"
#include "stats.h"
#include "geoip.h"

#define WEBSOCKET_PROTOCOL_TCP 0
#define WEBSOCKET_PROTOCOL_UDP 1

struct block *websocketKey2Accept(char *data, int size);

void deleteWebsocket(struct list *websockets, int socket);

void broadcast(struct list *websockets, struct geoip *geoip, struct in6_addr ip, struct stats *stats, unsigned char protocol);

#endif //SC6_WEBSOCKET_H
