CC=gcc
SERVER_FILES=server.c queue.c sem.c alloc.c uri.c socket.c string.c thread_client_tcp.c time.c block.c \
	stats.c socket_tcp.c socket_udp.c thread_garbage.c data_structure.c data_garbage.c thread_client_udp.c \
	socket_udp_structure.c equeue.c socket_garbage.c interval.c udp_request.c rps.c list.c data.c basic.c \
	base64.c argument.c thread.c
SERVER_OUTPUT=-o etracker
SERVER_CFLAGS=-pthread -lm
FSANITIZE_ADDRESS=-g -fsanitize=address
FSANITIZE_THREAD=-g -fsanitize=thread
REVISION=`test -d .git && git rev-parse --short HEAD`
RM_SERVER=rm -rf etracker.dSYM
RM_CLIENT=rm -rf client.o.dSYM
OPTIMIZE=-march=native -O2 -pipe

all:
	$(RM_SERVER)
	$(RM_CLIENT)
	$(CC) $(SERVER_FILES) $(SERVER_OUTPUT) $(SERVER_CFLAGS) -DREVISION=\"$(REVISION)\"
	$(CC) client.c -o client.o
tidy:
	$(RM_SERVER)
	$(CC) $(SERVER_FILES) $(SERVER_OUTPUT) $(SERVER_CFLAGS) -DREVISION=\"$(REVISION)\" -Wall -W -Werror \
		 $(FSANITIZE_ADDRESS)
tidy-max:
	$(RM_SERVER)
	$(CC) $(SERVER_FILES) $(SERVER_OUTPUT) $(SERVER_CFLAGS) -DREVISION=\"$(REVISION)\" -Wall -W -Werror \
		 -Wshadow -Wfloat-equal \
		 -pedantic -Wformat=2 -Wconversion \
		 $(FSANITIZE_ADDRESS)
server:
	$(RM_SERVER)
	$(CC) $(SERVER_FILES) $(SERVER_OUTPUT) $(SERVER_CFLAGS) -DREVISION=\"$(REVISION)\"
debug:
	$(RM_SERVER)
	$(CC) $(SERVER_FILES) $(SERVER_OUTPUT) $(SERVER_CFLAGS) -DREVISION=\"$(REVISION)\" $(FSANITIZE_ADDRESS)
client:
	$(RM_CLIENT)
	$(CC) -Wall -W -Werror client.c -o client.o
test:
	$(CC) -Werror \
		test.c queue.c alloc.c block.c list.c sem.c base64.c \
		-o test.o
clear:
	$(RM_SERVER)
	$(RM_CLIENT)
	rm *.o
