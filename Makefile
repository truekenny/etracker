all:
	rm -rf server.o.dSYM
	rm -rf client.o.dSYM
	gcc -pthread server.c queue.c sem.c alloc.c uri.c socket.c string.c thread_client_tcp.c data_change.c time.c block.c stats.c socket_tcp.c socket_udp.c thread_garbage.c data_structure.c data_sem.c data_garbage.c data_torrent_stat.c data_render.c thread_client_udp.c socket_udp_response_structure.c data_get.c -o server.o -lm
	gcc client.c -o client.o
tidy:
	rm -rf server.o.dSYM
	gcc -Wall -W -Werror -pthread server.c queue.c sem.c alloc.c uri.c socket.c string.c thread_client_tcp.c data_change.c time.c block.c stats.c socket_tcp.c socket_udp.c thread_garbage.c data_structure.c data_sem.c data_garbage.c data_torrent_stat.c data_render.c thread_client_udp.c socket_udp_response_structure.c data_get.c -o server.o -lm
	gcc -Wall -W -Werror client.c -o client.o
server:
	rm -rf server.o.dSYM
	gcc -pthread server.c queue.c sem.c alloc.c uri.c socket.c string.c thread_client_tcp.c data_change.c time.c block.c stats.c socket_tcp.c socket_udp.c thread_garbage.c data_structure.c data_sem.c data_garbage.c data_torrent_stat.c data_render.c thread_client_udp.c socket_udp_response_structure.c data_get.c -o server.o -lm
client:
	rm -rf client.o.dSYM
	gcc client.c -o client.o
test:
	gcc test.c queue.c alloc.c block.c -o test.o
clear:
	rm -rf server.o.dSYM
	rm -rf client.o.dSYM
	rm *.o
