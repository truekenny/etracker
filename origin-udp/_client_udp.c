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
/*
    char *hello = "\x11\x00\x00\x00\x00\x00\x00\x22"
                  "\x00\x00\x00\x02"
                  "\x33\x00\x00\x44"
                  "11112222333344445555"
                  "66667777888899990000"
                  ;
    int helloSize = 56;
*/
    // announce
    char *hello = "12345678" // con_id
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
                  "\x10\x10" // port
                  ;
    int helloSize = 98;
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

    sendto(sockfd, (const char *) hello, helloSize,
           MSG_CONFIRM, (const struct sockaddr *) &servaddr,
           sizeof(servaddr));
    printf("Hello message sent.\n");

    n = recvfrom(sockfd, (char *) buffer, MAXLINE,
                 MSG_WAITALL, (struct sockaddr *) &servaddr,
                 (unsigned int *) &len);
    buffer[n] = '\0';
    printf("Server : %s\n", buffer);

    printHex(buffer, n);

    close(sockfd);
    return 0;
}
