// Client side implementation of UDP client-server model
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "../string.h"

#define PORT        3000
#define MAXLINE     1024
#define MSG_CONFIRM_ 0

/**
 * 1 - announce event, 2 - started, 3 - stopped
 * 2 - announce peer
 * 3 - announce hash
 * 4 - scrape hash 1
 * 5 - scrape hash 2
 *
 * 11111111111111111111
 * 22222222222222222222
 * 33333333333333333333
 * 44444444444444444444
 *
 */
int main(int argc, char *argv[]) {
    int sockfd;
    int ipv6 = argc > 6;

    char buffer[MAXLINE];

    char *hello2[3] = {
            // connect
            "\x00\x00\x04\x17\x27\x10\x19\x80" // magic
            "\x00\x00\x00\x00" // action = connect = 0
            "abcd", // trans_id


            // announce
            "12345678" // con_id
            "\x00\x00\x00\x01" // action
            "acdb" // trans_id
            "11112222333344445555" // info
            "-UDP-777888899990000" // peer
            "\x00\x00\x00\x00\x00\x00\x00\x00" // down
            "\x00\x00\x00\x00\x00\x00\x00\x00" // left
            "\x00\x00\x00\x00\x00\x00\x00\x00" // up
            "\x00\x00\x00\x02" // event
            "bbbb" // ip
            "cccc" // key
            "\x00\x00\x00\x21" // num
            "\x10\x11", // Формат network, port = 0x1011 = 4113


            // scrape
            "\x88\x77\x66\x55\x44\x33\x11\x22" // conn_id
            "\x00\x00\x00\x02" // action
            "acdb" // transaction
            "11112222333344445555" // info 1
            "66667777888899990000" // info 2
    };
    int helloSize[3] = {16, 98, 56};

    char *hello[3];
    for (int i = 0; i < 3; ++i) {
        hello[i] = malloc(helloSize[i]);

        void *src = &hello2[i][0];
        void *dst = &hello[i][0];
        int size = helloSize[i];

        // printf("%p -> %p (%d)\n", src, dst, size);
        // printf("src:\n");
        // printHex(src, size);
        memcpy(dst, src, size);
        // printf("dst:\n");
        // printHex(dst, size);
        // printf("end\n");
    }

    // Dynamic data
    hello[1][83] = atoi(argv[1]);
    // peer
    memcpy(&hello[1][36], argv[2], 20);
    // hash
    memcpy(&hello[1][16], argv[3], 20);
    memcpy(&hello[2][16], argv[4], 20);
    memcpy(&hello[2][36], argv[5], 20);

    for (int i = 0; i <= 2; i++) {
        struct sockaddr_in servaddr;
        struct sockaddr_in6 servaddr6;

        if ((sockfd = socket(ipv6 ? AF_INET6 : AF_INET, SOCK_DGRAM, 0)) < 0) {
            perror("socket creation failed");
            exit(EXIT_FAILURE);
        }

        memset(&servaddr, 0, sizeof(servaddr));
        servaddr.sin_family = AF_INET;
        servaddr.sin_port = htons(PORT);
        servaddr.sin_addr.s_addr = INADDR_ANY;
        memset(&servaddr6, 0, sizeof(servaddr6));
        servaddr6.sin6_family = AF_INET6;
        servaddr6.sin6_port = htons(PORT);
        servaddr6.sin6_addr = in6addr_loopback;


        int n, len;

        if (argc > 1) {
            printf("Send: index=%d size=%d\n", i, helloSize[i]);
            printHex(hello[i], helloSize[i]);
        }

        if (ipv6) {
            printf("sendto ipv6\n");
            sendto(sockfd, (const char *) hello[i], helloSize[i], MSG_CONFIRM_,
                   (const struct sockaddr *) &servaddr6, sizeof(servaddr6));
        } else {
            sendto(sockfd, (const char *) hello[i], helloSize[i], MSG_CONFIRM_,
                   (const struct sockaddr *) &servaddr, sizeof(servaddr));
        }

        if (ipv6) {
            printf("fecvfrom ipv6\n");
            len = sizeof(servaddr6);
            n = recvfrom(sockfd, (char *) buffer, MAXLINE, MSG_WAITALL,
                         (struct sockaddr *) &servaddr6, (unsigned int *) &len);
        } else {
            len = sizeof(servaddr);
            n = recvfrom(sockfd, (char *) buffer, MAXLINE, MSG_WAITALL,
                         (struct sockaddr *) &servaddr, (unsigned int *) &len);
        }

        buffer[n] = '\0';
        if (argc > 1) {
            printf("Receive: index=%d size=%d\n", i, n);
            printHex(buffer, n);
        }
        close(sockfd);
    }

    return 0;
}
