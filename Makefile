all:
	gcc -pthread server_auto.c -o server_auto.o
	gcc client_auto.c -o client_auto.o
server:
	gcc -pthread server_auto.c -o server_auto.o
client:
	gcc client_auto.c -o client_auto.o
