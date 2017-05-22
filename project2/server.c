/* Jack Kaloger 758278
 * COMPUTER SYSTEMS PROJECT 2
 * COMP30023 2017
 * server.c
 */

#include "server.h"
#include "uint256.h"
#include "sha256.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdint.h>

/* Start Server */
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

/* Thread Entry */
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

/* PING message handler */
void ping_handler(int sockfd)
{
    int n = write(sockfd, "PONG\r\n", 40);
    if(n < 4) {
        perror("ERROR writing to socket");
        close(sockfd);
        pthread_exit(NULL);
    }
}

/* PONG message handler */
void pong_handler(int sockfd)
{
    int n = write(sockfd, "ERRO\tPONG strictly for server use     \r\n", 40);
    if(n < 35) {
        perror("ERROR writing to socket");
        close(sockfd);
        pthread_exit(NULL);
    }
}

/* OKAY message handler */
void okay_handler(int sockfd)
{
    int n = write(sockfd, "ERRO\tOKAY is not okay                 \r\n", 40);
    if(n < 23) {
        perror("ERROR writing to socket");
        close(sockfd);
        pthread_exit(NULL);
    }
}

/* SOLN message handler */
void soln_handler(int sockfd)
{
    /* counting vars */
    int n;

    /* parsing vars */
    BYTE diff_stream[10]; // 8 byte hex + 2 whitespace padding
    uint32_t difficulty;

    BYTE seed_stream[65]; // 64 byte seed with null byte end
    BYTE seed[33];

    BYTE soln_stream[18]; // 16 byte hex + 2 whitespace padding
    uint64_t solution;

    BYTE concat[40];

    /* read in difficulty */
    bzero(diff_stream, 10);
    n = read(sockfd, &diff_stream, 10); // read in the hex string padded by a space on either side
    if (n < 0) {
        perror("ERROR reading from socket");
        close(sockfd);
        pthread_exit(NULL);
    } else if(n < 10) {
        perror("ERROR malformed message contents");
        close(sockfd);
        pthread_exit(NULL);
    }

    difficulty = (uint32_t)strtoul(diff_stream, NULL, 16); // hex2uint32

    /* read in seed */
    bzero(seed_stream, 65); // \0 will be written to the end
    n = read(sockfd, seed_stream, 64); // read in our seed
    if(n < 0) {

    } else if(n < 64) {

    }

    // convert hex string to hex values (64 byte character array to 32 byte hex)
    BYTE *current = seed_stream;
    bzero(seed, 33);
    for(int i = 0 ; i < 32 ; i++) {
        BYTE tmp[3]; // tmp var to store hex chars
        bzero(tmp, 3);
        sprintf(tmp, "%c%c", current[0], current[1]); // load hex chars into tmp var
        uint8_t hex = strtol(tmp, NULL, 16);
        current += 2;
    }

    /* read in solution */
    bzero(soln_stream, 18);
    n = read(sockfd, soln_stream, 17); // read in the hex string padded by a space at index 0
    if (n < 0) {
        perror("ERROR reading from socket");
        close(sockfd);
        pthread_exit(NULL);
    } else if(n < 17) {
        perror("ERROR malformed message contents");
        close(sockfd);
        pthread_exit(NULL);
    }

    solution = strtoull(soln_stream, NULL, 16);

    bzero(concat, 40);
    // concatenate seed and solution
    memcpy(concat, seed, 32);
    memcpy(concat+32, &solution, 8);

    /* crypto vars */
    SHA256_CTX ctx; // md5 structure that holds hash-related data
    BYTE y[SHA256_BLOCK_SIZE]; // hash output variable SHA256_BLOCK_SIZE is defined in sha256.h

    sha256_init(&ctx); // initialise the CTX object
    sha256_update(&ctx, concat, 80); // add the data to be hashed to the hashing object
    sha256_final(&ctx, y); // hash the data! outputs to hash variable
    /* hash again */
    sha256_init(&ctx); // initialise the CTX object
    sha256_update(&ctx, y, SHA256_BLOCK_SIZE); // add the data to be hashed to the hashing object
    sha256_final(&ctx, y); // hash the data! outputs to hash variable

    uint32_t a = difficulty & 0xFF000000;
    uint32_t b = difficulty & 0x00FFFFFF;
    uint32_t target = b * 2^(8*(a-3));

    if(y < target) {
        n = write(sockfd, "OKAY\r\n", 6);
        if(n < 0) {
            perror("ERROR writing to socket");
            close(sockfd);
            pthread_exit(NULL);
        } else if(n < 6) {
            perror("ERROR write error");
            close(sockfd);
            pthread_exit(NULL);
        }
    } else {
        n = write(sockfd, "ERRO\tInvalid proof of work            \r\n", 44);
        if(n < 0) {
            perror("ERROR writing to socket");
            close(sockfd);
            pthread_exit(NULL);
        } else if(n < 44) {
            perror("ERROR write error");
            close(sockfd);
            pthread_exit(NULL);
        }
    }

    close(sockfd);
}

/* WORK message handler */
void work_handler(int sockfd)
{

}