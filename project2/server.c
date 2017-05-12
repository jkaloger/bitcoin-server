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
#include <pthread.h>

void entry_point(void *arg);

void runServer(int portno) {

    int sockfd, newsockfd, clilen;
    struct sockaddr_in serv_addr, cli_addr;
    int n  = 0;

    /* Create TCP socket */

    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0) {
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
             sizeof(serv_addr)) < 0) {
        perror("ERROR on binding");
        exit(1);
    }

    /* Listen on socket - means we're ready to accept connections -
     incoming connection requests will be queued */

    listen(sockfd, 5);

    clilen = sizeof(cli_addr);

    /* Accept a connection - block until a connection is ready to
         be accepted. Get back a new file descriptor to communicate on. */
    while(newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr,
                       &clilen)) {
        pthread_t tid;
        if(pthread_create(&tid, NULL, entry_point, &newsockfd) != 0) {
            printf("Error creating thread");
            exit(1);
        }
    }

    perror("ERROR on accept");
    close(sockfd);
    exit(1);

}

void printMalformedError(int fd)
{
    int n = write(fd, "ERRO\tmalformed header\r\n", 23);
    if (n < 0) {
        perror("ERROR writing to socket");
        exit(1);
    }
}

void entry_point(void *arg)
{
    char buffer[256], header[6];
    bzero(buffer, 256);
    bzero(header, 6);

    // read counter
    int n = 0;
    int sockfd = *(int *)arg;

    /* Read characters from the connection,
        then process */

    n = read(sockfd, header, 6);
    if (n < 0) {
        perror("ERROR reading from socket");
        close(sockfd);
        pthread_exit(NULL);
    }


    /* check the header is valid */
    if (n < 6) {
        printMalformedError(sockfd);
        close(sockfd);
        pthread_exit(NULL);
    }

    /* parse the header */
    if (strncmp(header, "PING\r\n", 6) == 0) {
        n = write(sockfd, "PONG", 4);
    } else if (strncmp(header, "PONG\r\n", 6) == 0) {
        n = write(sockfd, "ERRO\tPONG strictly for server use\r\n", 35);
    } else if (strncmp(header, "OKAY\r\n", 6) == 0) {
        n = write(sockfd, "ERRO\tOKAY is not okay\r\n", 23);
    } else {
        n = write(sockfd, "ERRO\tInvalid server usage\r\n", 27);
    }

    if (n < 0) {
        perror("ERROR writing to socket");
        close(sockfd);
        pthread_exit(NULL);
    }

    /* close socket */
    close(sockfd);

    pthread_exit(NULL);

}