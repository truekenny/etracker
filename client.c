#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#define MAX_SIZE 100
#define PAUSE_TIME 1

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("./client host port\n");
        return 1;
    }

    int sock_desc;
    struct sockaddr_in serv_addr;
    char sbuff[MAX_SIZE], rbuff[MAX_SIZE];
    long int ttime;
    int n;

    if ((sock_desc = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        printf("Failed creating socket\n");

    bzero((char *) &serv_addr, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_addr.sin_port = htons(atoi(argv[2]));


    // Timeout
    struct timeval tv;
    tv.tv_sec = 60;
    tv.tv_usec = 0;
    if (setsockopt(sock_desc, SOL_SOCKET, SO_RCVTIMEO, (const char *) &tv, sizeof tv) < 0) {
        printf("Failed to set SO_RCVTIMEO\n");
        return -1;
    }

    if (setsockopt(sock_desc, SOL_SOCKET, SO_SNDTIMEO, (const char *) &tv, sizeof tv) < 0) {
        printf("Failed to set SO_SNDTIMEO\n");
        return -1;
    }

    if (connect(sock_desc, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        printf("Failed to connect to server\n");
        return -1;
    }


    struct sockaddr_in sin;
    int len = sizeof(sin);
    if (getsockname(sock_desc, (struct sockaddr *) &sin, (socklen_t * ) & len) == -1)
        printf("Port number failed\n");
    else
        printf("Local port number %d\n", ntohs(sin.sin_port));

    printf("Connected successfully\n");
    while (1) {
        ttime = time(NULL);
        sprintf(sbuff, "%s:%d %.24s %d\n", argv[1], ntohs(sin.sin_port), ctime(&ttime), rand());

        if (send(sock_desc, sbuff, strlen(sbuff), 0) == -1) {
            perror("Send error");
            break;
        }
        printf("< %s", sbuff);

        memset(rbuff, 0, MAX_SIZE);
        if ((n = recv(sock_desc, rbuff, MAX_SIZE, 0)) <= 0) {
            printf("Error %d\n", n);
            break;
        } else
            printf("> %s", rbuff);

        bzero(rbuff, MAX_SIZE); //to clean buffer-->IMP otherwise previous word characters also came
        sleep(PAUSE_TIME);
    }

    close(sock_desc);

    return 0;

}
