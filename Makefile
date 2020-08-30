all:
	rm -rf server.o.dSYM
	rm -rf client.o.dSYM
	gcc -pthread server.c queue.c sem.c alloc.c -o server.o
	gcc client.c -o client.o
server:
	rm -rf server.o.dSYM
	gcc -pthread server.c queue.c sem.c alloc.c -o server.o -g
client:
	rm -rf client.o.dSYM
	gcc client.c -o client.o -g
clear:
	rm -rf server.o.dSYM
	rm -rf client.o.dSYM
	rm *.o
