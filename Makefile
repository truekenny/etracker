all:
	rm -rf server.o.dSYM
	rm -rf client.o.dSYM
	gcc -pthread server.c queue.c sem.c alloc.c uri.c socket.c string.c thread.c data.c time.c block.c stats.c socket_tcp.c socket_udp.c -o server.o
	gcc client.c -o client.o
server:
	rm -rf server.o.dSYM
	gcc -pthread server.c queue.c sem.c alloc.c uri.c socket.c string.c thread.c data.c time.c block.c stats.c socket_tcp.c socket_udp.c -o server.o
client:
	rm -rf client.o.dSYM
	gcc client.c -o client.o
test:
	gcc test.c queue.c alloc.c block.c -o test.o
clear:
	rm -rf server.o.dSYM
	rm -rf client.o.dSYM
	rm *.o
