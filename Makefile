all:
	gcc -pthread server.c -o server.o
	gcc client.c -o client.o
server:
	gcc -pthread server.c -o server.o
client:
	gcc client.c -o client.o
clear:
	rm *.o
