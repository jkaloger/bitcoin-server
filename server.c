/* Jack Kaloger 758278
 * COMPUTER SYSTEMS PROJECT 2
 * COMP30023 2017
 * server.c
 */

#include "server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

void runServer(int portno)
{

    int sockfd, newsockfd, clilen;
    char buffer[256];
    struct sockaddr_in serv_addr, cli_addr;
    int n;

    /* Create TCP socket */

    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0)
    {
        perror("ERROR opening socket");
        exit(1);
    }


    bzero((char *) &serv_addr, sizeof(serv_addr));

    /* Create address we're going to listen on (given port number)
     - converted to network byte order & any IP address for
     this machine */

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);  // store in machine-neutral format

    /* Bind address to the socket */

    if (bind(sockfd, (struct sockaddr *) &serv_addr,
             sizeof(serv_addr)) < 0)
    {
        perror("ERROR on binding");
        exit(1);
    }

    /* Listen on socket - means we're ready to accept connections -
     incoming connection requests will be queued */

    listen(sockfd, 5);

    clilen = sizeof(cli_addr);

    // loop
    while(1) {

        /* Accept a connection - block until a connection is ready to
         be accepted. Get back a new file descriptor to communicate on. */

        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr,
                           &clilen);

        if (newsockfd < 0) {
            perror("ERROR on accept");
            exit(1);
        }

        bzero(buffer, 256);

        /* Read characters from the connection,
            then process */

        n = read(newsockfd, buffer, 255);

        if (n < 0) {
            perror("ERROR reading from socket");
            exit(1);
        }

        printf("Here is the message: %s\n", buffer);

        if(strncmp(buffer, "PING", 4) == 0)
            n = write(newsockfd, "PONG\r\n", 6);
        else if(strncmp(buffer, "PONG", 4) == 0)
            n = write(newsockfd, "ERRO\t\tPONG strictly for server use\r\n", 42);
        else
            n = write(newsockfd, "MESSAGE RECEIVED\r\n", 18);

        if (n < 0) {
            perror("ERROR writing to socket");
            exit(1);
        }

        /* close socket */
        close(newsockfd);
    }

    /* close socket */
    close(sockfd);

}