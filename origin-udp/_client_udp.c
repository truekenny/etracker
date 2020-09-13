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
#define MSG_CONFIRM 0

// Driver code
int main() {
    int sockfd;
    char buffer[MAXLINE];
    // scrape
    // announce
    char *hello[3] = {
            // connect
            "\x00\x00\x04\x17\x27\x10\x19\x80" // magic
            "\x00\x00\x00\x00" // action = connect = 0
            "abcd", // trans_id


            // announce
            "12345678" // con_id
            "\x00\x00\x00\x01" // action
            "aaaa" // trans_id
            "11112222333344445555" // info
            "66667777888899990000" // peer
            "\x00\x00\x00\x00\x00\x00\x00\x00" // down
            "\x00\x00\x00\x00\x00\x00\x00\x00" // left
            "\x00\x00\x00\x00\x00\x00\x00\x00" // up
            "\x00\x00\x00\x02" // event
            "bbbb" // ip
            "cccc" // key
            "\x00\x00\x00\x20" // num
            "\x10\x10", // port

            // scrape
            "\x11\x00\x00\x00\x00\x00\x00\x22" // conn_id
            "\x00\x00\x00\x02" // action
            "\x33\x00\x00\x44" // transaction
            "11112222333344445555" // info 1
            "66667777888899990000" // info 2
    };
    int helloSize[3] = {16, 98, 56};


    for (int i = 0; i <= 2; i++) {
        struct sockaddr_in servaddr;

        // Creating socket file descriptor
        if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
            perror("socket creation failed");
            exit(EXIT_FAILURE);
        }

        memset(&servaddr, 0, sizeof(servaddr));

        // Filling server information
        servaddr.sin_family = AF_INET;
        servaddr.sin_port = htons(PORT);
        servaddr.sin_addr.s_addr = INADDR_ANY;

        int n, len;

        printf("Send : index=%d size=%d\n", i, helloSize[i]);
        printHex(hello[i], helloSize[i]);

        sendto(sockfd, (const char *) hello[i], helloSize[i],
               MSG_CONFIRM, (const struct sockaddr *) &servaddr,
               sizeof(servaddr));

        n = recvfrom(sockfd, (char *) buffer, MAXLINE,
                     MSG_WAITALL, (struct sockaddr *) &servaddr,
                     (unsigned int *) &len);
        buffer[n] = '\0';
        printf("Receive : %d\n", i);
        printHex(buffer, n);

        close(sockfd);
    }

    return 0;
}
