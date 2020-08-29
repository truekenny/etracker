all:
	rm -rf server.o.dSYM
	rm -rf client.o.dSYM
	gcc -pthread server.c -o server.o
	gcc client.c -o client.o
server:
	rm -rf server.o.dSYM
	gcc -pthread server.c queue.c sem.c -o server.o
client:
	rm -rf client.o.dSYM
	gcc client.c -o client.o
clear:
	rm -rf server.o.dSYM
	rm -rf client.o.dSYM
	rm *.o
