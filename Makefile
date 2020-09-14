CC=gcc
SERVER_FILES=server.c queue.c sem.c alloc.c uri.c socket.c string.c thread_client_tcp.c data_change.c time.c block.c \
	stats.c socket_tcp.c socket_udp.c thread_garbage.c data_structure.c data_sem.c data_garbage.c data_torrent_stat.c \
	data_render.c thread_client_udp.c socket_udp_structure.c data_get.c equeue.c socket_garbage.c interval.c \
	udp_request.c rps.c
SERVER_OUTPUT=-o server.o
SERVER_CFLAGS=-pthread -lm
REVISION=`test -d .git && git rev-parse --short HEAD`
RM_SERVER=rm -rf server.o.dSYM
RM_CLIENT=rm -rf client.o.dSYM

all:
	$(RM_SERVER)
	$(RM_CLIENT)
	$(CC) $(SERVER_FILES) $(SERVER_OUTPUT) $(SERVER_CFLAGS) -DREVISION=\"$(REVISION)\"
	$(CC) client.c -o client.o
tidy:
	$(RM_SERVER)
	$(CC) $(SERVER_FILES) $(SERVER_OUTPUT) $(SERVER_CFLAGS) -DREVISION=\"$(REVISION)\" -Ofast -Wall -W -Werror
	$(CC) -Wall -W -Werror client.c -o client.o
server:
	$(RM_SERVER)
	$(CC) $(SERVER_FILES) $(SERVER_OUTPUT) $(SERVER_CFLAGS) -DREVISION=\"$(REVISION)\" -Ofast
client:
	$(RM_CLIENT)
	$(CC) client.c -o client.o
test:
	$(CC) test.c queue.c alloc.c block.c -o test.o
clear:
	$(RM_SERVER)
	$(RM_CLIENT)
	rm *.o
