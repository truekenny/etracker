/**************************************************************************/
/* Header files needed for this sample program                            */
/**************************************************************************/
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>

/**************************************************************************/
/* Constants used by this program                                         */
/**************************************************************************/
#define SERVER_PORT     3005
#define BUFFER_LENGTH    250
#define FALSE              0

int main() {
    /***********************************************************************/
    /* Variable and structure definitions.                                 */
    /***********************************************************************/
    int sd = -1, sdconn = -1;
    int rc, on = 1, rcdsize = BUFFER_LENGTH;
    char buffer[BUFFER_LENGTH];
    struct sockaddr_in6 serveraddr, clientaddr;
    int addrlen = sizeof(clientaddr);
    char str[INET6_ADDRSTRLEN];

    /***********************************************************************/
    /* A do/while(FALSE) loop is used to make error cleanup easier.  The   */
    /* close() of each of the socket descriptors is only done once at the  */
    /* very end of the program.                                            */
    /***********************************************************************/
    do {

        /********************************************************************/
        /* The socket() function returns a socket descriptor, which represents   */
        /* an endpoint.  Get a socket for address family AF_INET6 to        */
        /* prepare to accept incoming connections on.                       */
        /********************************************************************/
        if ((sd = socket(AF_INET6, SOCK_STREAM, 0)) < 0) {
            perror("socket() failed");
            break;
        }

        /********************************************************************/
        /* The setsockopt() function is used to allow the local address to  */
        /* be reused when the server is restarted before the required wait  */
        /* time expires.                                                    */
        /********************************************************************/
        if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR,
                       (char *) &on, sizeof(on)) < 0) {
            perror("setsockopt(SO_REUSEADDR) failed");
            break;
        }

        /********************************************************************/
        /* After the socket descriptor is created, a bind() function gets a */
        /* unique name for the socket.  In this example, the user sets the  */
        /* address to in6addr_any, which (by default) allows connections to */
        /* be established from any IPv4 or IPv6 client that specifies port  */
        /* 3005. (that is, the bind is done to both the IPv4 and IPv6 TCP/IP    */
        /* stacks).  This behavior can be modified using the IPPROTO_IPV6   */
        /* level socket option IPV6_V6ONLY if required.                      */
        /********************************************************************/
        memset(&serveraddr, 0, sizeof(serveraddr));
        serveraddr.sin6_family = AF_INET6;
        serveraddr.sin6_port = htons(SERVER_PORT);
        /********************************************************************/
        /* Note: applications use in6addr_any similarly to the way they use */
        /* INADDR_ANY in IPv4.  A symbolic constant IN6ADDR_ANY_INIT also   */
        /* exists but can only be used to initialize an in6_addr structure  */
        /* at declaration time (not during an assignment).                  */
        /********************************************************************/
        serveraddr.sin6_addr = in6addr_any;
        /********************************************************************/
        /* Note: the remaining fields in the sockaddr_in6 are currently not */
        /* supported and should be set to 0 to ensure upward compatibility. */
        /********************************************************************/

        if (bind(sd,
                 (struct sockaddr *) &serveraddr,
                 sizeof(serveraddr)) < 0) {
            perror("bind() failed");
            break;
        }

        /********************************************************************/
        /* The listen() function allows the server to accept incoming       */
        /* client connections.  In this example, the backlog is set to 10.  */
        /* This means that the system will queue 10 incoming connection     */
        /* requests before the system starts rejecting the incoming         */
        /* requests.                                                        */
        /********************************************************************/
        if (listen(sd, 10) < 0) {
            perror("listen() failed");
            break;
        }

        printf("Ready for client connect().\n");

        /********************************************************************/
        /* The server uses the accept() function to accept an incoming      */
        /* connection request.  The accept() call will block indefinitely   */
        /* waiting for the incoming connection to arrive from an IPv4 or    */
        /* IPv6 client.                                                     */
        /********************************************************************/
        if ((sdconn = accept(sd, NULL, NULL)) < 0) {
            perror("accept() failed");
            break;
        } else {
            /*****************************************************************/
            /* Display the client address.  Note that if the client is       */
            /* an IPv4 client, the address will be shown as an IPv4 Mapped   */
            /* IPv6 address.                                                 */
            /*****************************************************************/
            getpeername(sdconn, (struct sockaddr *) &clientaddr, &addrlen);
            if (inet_ntop(AF_INET6, &clientaddr.sin6_addr, str, sizeof(str))) {
                printf("Client address is %s\n", str);
                printf("Client port is %d\n", ntohs(clientaddr.sin6_port));
            }
        }

        /********************************************************************/
        /* In this example we know that the client will send 250 bytes of   */
        /* data over.  Knowing this, we can use the SO_RCVLOWAT socket      */
        /* option and specify that we don't want our recv() to wake up      */
        /* until all 250 bytes of data have arrived.                        */
        /********************************************************************/
/*
        if (setsockopt(sdconn, SOL_SOCKET, SO_RCVLOWAT,
                       (char *) &rcdsize, sizeof(rcdsize)) < 0) {
            perror("setsockopt(SO_RCVLOWAT) failed");
            break;
        }
*/

        /********************************************************************/
        /* Receive that 250 bytes of data from the client                   */
        /********************************************************************/
        rc = recv(sdconn, buffer, sizeof(buffer), 0);
        if (rc < 0) {
            perror("recv() failed");
            break;
        }

        printf("%d bytes of data were received\n", rc);
        if (rc == 0 ||
            rc < sizeof(buffer)) {
            printf("The client closed the connection before all of the\n");
            printf("data was sent\n");
            break;
        }

        /********************************************************************/
        /* Echo the data back to the client                                 */
        /********************************************************************/
        rc = send(sdconn, buffer, sizeof(buffer), 0);
        if (rc < 0) {
            perror("send() failed");
            break;
        }

        /********************************************************************/
        /* Program complete                                                 */
        /********************************************************************/

    } while (FALSE);

    /***********************************************************************/
    /* Close down any open socket descriptors                              */
    /***********************************************************************/
    if (sd != -1)
        close(sd);
    if (sdconn != -1)
        close(sdconn);
}