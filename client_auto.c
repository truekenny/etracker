#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#define MAX_SIZE 50

int main(int argc, char *argv[])
{
   if(argc < 3) {
       printf("./client host port\n");
       return 1;
   }

    int sock_desc;
    struct sockaddr_in serv_addr;
    char sbuff[MAX_SIZE],rbuff[MAX_SIZE];
    long int ttime;

    if((sock_desc = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        printf("Failed creating socket\n");

    bzero((char *) &serv_addr, sizeof (serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_addr.sin_port = htons(atoi(argv[2]));


    // Timeout
    struct timeval tv;
    tv.tv_sec = 60;
    tv.tv_usec = 0;
    if(setsockopt(sock_desc, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv) < 0) {
        printf("Failed to set SO_RCVTIMEO\n");
        return -1;
    }

    if (connect(sock_desc, (struct sockaddr *) &serv_addr, sizeof (serv_addr)) < 0) {
        printf("Failed to connect to server\n");
        return -1;
    }


    struct sockaddr_in sin;
    int len = sizeof(sin);
    if (getsockname(sock_desc, (struct sockaddr *)&sin, &len) == -1)
        printf("Port number failed\n");
    else
        printf("Local port number %d\n", ntohs(sin.sin_port));


    int n;

    printf("Connected successfully - Please enter string\n");
    while(1 == 1)
    {
      ttime = time (NULL);
      sprintf(sbuff, "%s:%d %.24s\n", argv[1], ntohs(sin.sin_port), ctime (&ttime));

      send(sock_desc,sbuff,strlen(sbuff),0);
      printf("< %s", sbuff);

          if((n = recv(sock_desc,rbuff,MAX_SIZE,0)) <= 0){
             printf("Error %d\n", n);
             break;
          }
          else
           printf("> %s", rbuff);

       bzero(rbuff,MAX_SIZE);//to clean buffer-->IMP otherwise previous word characters also came
       sleep(10);
    }
        close(sock_desc);
    return 0;

}
