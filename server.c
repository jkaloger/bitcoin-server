/* Jack Kaloger 758278
 * COMPUTER SYSTEMS PROJECT 2
 * COMP30023 2017
 * server.c
 */

#include "server.h"
#include "uint256.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdint.h>

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
            perror("ERROR creating thread");
            exit(1);
        }

        pthread_join(tid, NULL);
        close(newsockfd);
        close(sockfd);
        exit(0);
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

void entry_point(int *arg)
{
    char header[4];
    bzero(header, 4);

    // read counter
    int n = 0;
    int sockfd = *arg;

    /* Read characters from the connection,
        then process */

    n = read(sockfd, header, 4);
    if (n < 0) {
        perror("ERROR reading from socket");
        close(sockfd);
        pthread_exit(NULL);
    }


    /* check the header is valid */
    if (n < 4) {
        printMalformedError(sockfd);
        close(sockfd);
        pthread_exit(NULL);
    }

    /* parse the header */
    if (strncmp(header, "PING", 4) == 0)
        ping_handler(sockfd);
    else if (strncmp(header, "PONG", 4) == 0)
        pong_handler(sockfd);
    else if (strncmp(header, "OKAY", 4) == 0)
        okay_handler(sockfd);
    else if (strncmp(header, "SOLN", 4) == 0)
        soln_handler(sockfd);
    else if (strncmp(header, "WORK", 4) == 0)
        work_handler(sockfd);
    else
        n = write(sockfd, "ERRO\tInvalid server usage\r\n", 27);

    if (n < 0) {
        perror("ERROR writing to socket");
        close(sockfd);
        pthread_exit(NULL);
    }

    /* close socket */
    close(sockfd);

    pthread_exit(NULL);

}

void ping_handler(int sockfd)
{
    int n = write(sockfd, "PONG\r\n", 6);
    if(n < 4) {
        perror("ERROR writing to socket");
        close(sockfd);
        pthread_exit(NULL);
    }
}
void pong_handler(int sockfd)
{
    int n = write(sockfd, "ERRO\tPONG strictly for server use\r\n", 35);
    if(n < 35) {
        perror("ERROR writing to socket");
        close(sockfd);
        pthread_exit(NULL);
    }
}
void okay_handler(int sockfd)
{
    int n = write(sockfd, "ERRO\tOKAY is not okay\r\n", 23);
    if(n < 23) {
        perror("ERROR writing to socket");
        close(sockfd);
        pthread_exit(NULL);
    }
}
void soln_handler(int sockfd)
{
    /* counting vars */
    int n;

    /* parsing vars */
    BYTE diff_stream[10]; // 8 byte hex + 2 whitespace padding
    uint32_t difficulty;

    BYTE seed[64]; // 64 byte seed

    BYTE soln_stream[18]; // 16 byte hex + 2 whitespace padding
    uint64_t solution;

    /* read in difficulty */
    bzero(diff_stream, 10);
    n = read(sockfd, &diff_stream, 10); // read in the hex string padded by a space on either side
    if (n < 0) {
        perror("ERROR reading from socket");
        close(sockfd);
        pthread_exit(NULL);
    }

    difficulty = (uint32_t)strtoul(diff_stream, NULL, 16); // hex2uint32 (network order)

    /* read in seed */
    n = read(sockfd, seed, 64); // read in our seed

    /* read in solution */
    bzero(soln_stream, 17);
    n = read(sockfd, soln_stream, 17); // read in the hex string padded by a space at index 0
    if (n < 0) {
        perror("ERROR reading from socket");
        close(sockfd);
        pthread_exit(NULL);
    }
    solution = strtoull(soln_stream, NULL, 16);
    printf("SOLN: %llu", solution);

    close(sockfd);
}
void work_handler(int sockfd)
{

}