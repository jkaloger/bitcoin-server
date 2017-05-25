/* Jack Kaloger 758278
 * COMPUTER SYSTEMS PROJECT 2
 * COMP30023 2017
 * server.c
 */

#include "server.h"
#include "queue.h"
#include "uint256.h"
#include "sha256.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <stdint.h>
#include <time.h>

/* SHARED BETWEEN THREADS */
// global work queue
Queue work_queue;

// log file
FILE *log_file;

/* Start Server */
void runServer(int portno) {
    int sockfd;
    struct sockaddr_in serv_addr;

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

    server_loop(sockfd);

    perror("ERROR unknown error caused server to stop");
    close(sockfd);
    exit(1);

}

void server_loop(int sockfd)
{
    struct sockaddr_in cli_addr;
    int clilen = sizeof(cli_addr);
    int newsockfd;

    /* Log file */
    log_file = fopen("./log.txt",  "w");

    /* Accept a connection. Get back a new file descriptor to communicate on. */
    while((newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr,
                              &clilen))) {
        // create new thread and pass the file descriptor
        pthread_t tid;
        if(pthread_create(&tid, NULL, entry_point, &newsockfd) != 0) {
            perror("ERROR creating thread");
            exit(1);
        }
    }
}

void printMalformedError(int sockfd)
{
    write_error(sockfd, "Malformed header");
}

/* Thread Entry */
void entry_point(void *arg) {
    pthread_detach(pthread_self());
    char header[4];
    bzero(header, 4);

    // read counter
    int n = 0;
    int sockfd = *(int *)arg;

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

    /* TODO : PARSE FULL MESSAGE BEFORE SENDING -- ADDING TO LOG */

    /* parse the header */
    if (strncmp(header, "PING", 4) == 0) {
        server_log(sockfd, "PING\r\n", 0);
        ping_handler(sockfd);
    } else if (strncmp(header, "PONG", 4) == 0) {
        server_log(sockfd, "PONG\r\n", 0);
        pong_handler(sockfd);
    } else if (strncmp(header, "OKAY", 4) == 0) {
        server_log(sockfd, "OKAY\r\n", 0);
        okay_handler(sockfd);
    } else if (strncmp(header, "SOLN", 4) == 0) {
        server_log(sockfd, "SOLN\r\n", 0);
        soln_handler(sockfd);
    } else if (strncmp(header, "WORK", 4) == 0) {
        server_log(sockfd, "WORK\r\n", 0);
        work_handler(sockfd);
    }
    else
        n = write(sockfd, "ERRO\tInvalid server usage\r\n", 27);

    if (n < 0) {
        perror("ERROR writing to socket");
        close(sockfd);
        pthread_exit(NULL);
    }

    close(sockfd);
    pthread_exit(NULL);

}

/* PING message handler */
void ping_handler(int sockfd)
{
    line_end_check(sockfd);
    char *pong = "PONG\r\n";
    write_msg(sockfd, pong, 6);
}

/* PONG message handler */
void pong_handler(int sockfd)
{
    line_end_check(sockfd);
    write_error(sockfd, "PONG strictly for server use");
}

/* OKAY message handler */
void okay_handler(int sockfd)
{
    line_end_check(sockfd);
    write_error(sockfd, "OKAY is not okay");
}

/* SOLN message handler */
void soln_handler(int sockfd)
{
    /* counting vars */
    int n;

    /* parsing vars */
    char diff_stream[10]; // 8 byte hex + 2 whitespace padding
    char seed_stream[65]; // 64 byte seed with null byte end
    char soln_stream[18]; // 16 byte hex + 2 whitespace padding

    /* read in difficulty */
    bzero(diff_stream, 10);
    n = read(sockfd, &diff_stream, 10); // read in the hex string padded by a space on either side
    if (n < 0) {
        perror("ERROR reading from socket");
        close(sockfd);
        pthread_exit(NULL);
    } else if(n < 10) {
        write_error(sockfd, "malformed difficulty");
        close(sockfd);
        pthread_exit(NULL);
    }

    /* read in seed */
    bzero(seed_stream, 65); // \0 will be written to the end
    n = read(sockfd, seed_stream, 64); // read in our seed
    if(n < 0) {

    } else if(n < 64) {

    }

    /* read in solution */
    bzero(soln_stream, 18);
    n = read(sockfd, soln_stream, 17); // read in the hex string padded by a space at index 0
    if (n < 0) {
        perror("ERROR reading from socket");
        close(sockfd);
        pthread_exit(NULL);
    } else if(n < 17) {
        write_error(sockfd, "malformed solution");
    }

    line_end_check(sockfd);

    if(check_proof(diff_stream, seed_stream, soln_stream) < 0) {
        write_msg(sockfd, "OKAY\r\n", 6);
    } else {
        write_error(sockfd, "invalid proof of work");
    }

    close(sockfd);
}

/* WORK message handler */
void work_handler(int sockfd)
{
    /* counting vars */
    int n;

    /* parsing vars */
    BYTE diff_stream[10]; // 8 byte hex string + 2 whitespace padding
    BYTE seed_stream[65]; // 64 byte seed with null byte end
    BYTE start_stream[18]; // 16 byte hex string + 2 whitespace padding
    BYTE worker_stream[3]; // 2 byte hex string + 1 whitespace padding
}

BYTE *hex2int(int num_bytes, char *bytestream)
{
    // convert hex string to hex values
    BYTE *output = malloc(sizeof(BYTE) * (num_bytes + 1));
    char *current = bytestream;
    bzero(output, num_bytes + 1);
    for(int i = 0 ; i < num_bytes ; i++) {
        char tmp[3]; // tmp var to store hex chars
        bzero(tmp, 3);
        sprintf(tmp, "%c%c", current[0], current[1]); // load hex chars into tmp var
        uint8_t hex = strtoul((const char *)tmp, NULL, 16);
        output[i] = hex;
        current += 2 * sizeof(char);
    }

    return output;
}

int check_proof(char *diff_stream, char *seed_stream, char *soln_stream)
{
    // vars
    uint32_t difficulty;
    BYTE *seed;
    BYTE *solution;
    BYTE concat[40];

    difficulty = (uint32_t)strtoul((const char *)diff_stream, NULL, 16); // hex2uint32
    difficulty = htonl(difficulty);
    // convert hex string to hex values (64 byte character array to 32 byte hex/uint256)
    seed = hex2int(32, seed_stream);

    //solution = strtoull(soln_stream, NULL, 16);
    solution = hex2int(8, soln_stream + sizeof(char));

    bzero(concat, 40);
    // concatenate seed and solution
    memcpy(concat, seed, 32);
    /* TODO ENDIANNESS ? */
    //solution = htonl(solution);
    memcpy(concat+32, solution, 8);

    /* crypto vars */
    SHA256_CTX ctx; // md5 structure that holds hash-related data
    BYTE y[SHA256_BLOCK_SIZE]; // hash output variable SHA256_BLOCK_SIZE is defined in sha256.h

    sha256_init(&ctx); // initialise the CTX object
    sha256_update(&ctx, concat, 40); // add the data to be hashed to the hashing object
    sha256_final(&ctx, y); // hash the data! outputs to hash variable
    /* hash again */
    sha256_init(&ctx); // initialise the CTX object
    sha256_update(&ctx, y, SHA256_BLOCK_SIZE); // add the data to be hashed to the hashing object
    sha256_final(&ctx, y); // hash the data! outputs to hash variable

    unsigned  mask;
    uint8_t a = difficulty & 0x000000FF;
    a = 8*(a - 3);
    mask = ((1 << 24) - 1) << 8;
    BYTE beta_stream[65];
    uint32_t b = difficulty & mask;
    b = htonl(b);
    sprintf(beta_stream, "%064x", b); // 64 chars => 32 byte array
    BYTE target[32];
    BYTE *beta;
    beta = hex2int(32, beta_stream);


    uint256_init(target);
    uint256_sl(target, beta, (BYTE)a);

    int result = memcmp(y, target, 32);

    return result;

}

void line_end_check(int sockfd)
{
    int n;
    BYTE final[2];
    n = read(sockfd, &final,2);
    /* TODO: FIX THIS */
    if(n < 2 || strncmp((const char *)final, "\r\n", 2) != 0) {
        write_error(sockfd, "invalid line endings");
        close(sockfd);
        pthread_exit(NULL);
    }
}

void write_error(int sockfd, char *str)
{
    char err[48];
    sprintf((char *)&err, "ERRO %-40s\r\n", str);
    int n = write(sockfd, err, 48);
    if(n < 48) {
        perror("ERROR writing to socket");
        close(sockfd);
        pthread_exit(NULL);
    }
    server_log(sockfd, err, 1);
}

void write_msg(int sockfd, char *str, int strlen)
{
    int n;
    n = write(sockfd, str, strlen);
    if(n < strlen) {
        perror("ERROR writing to socket");
        close(sockfd);
        pthread_exit(NULL);
    }
    server_log(sockfd, str, 1);
}

void server_log(int sockfd, char *exchange, int is_server)
{
    char msg[1024];
    time_t now = time(0);
    char time[20];
    strftime(time, 20, "%Y-%m-%d %H:%M:%S", localtime(&now));

    // get ip addr
    char *ip;
    if(is_server) {
        ip = "0.0.0.0";
    } else {
        struct sockaddr_in addr;
        socklen_t addr_size = sizeof(struct sockaddr_in);
        getpeername(sockfd, (struct sockaddr *) &addr, &addr_size);
        ip = inet_ntoa(addr.sin_addr);
    }

    // generate/write message
    sprintf((char *)&msg, "[%s] %02d %-16s %s", time, sockfd, ip, exchange);
    fprintf(log_file, "%s", msg);
    fflush(log_file); // force write to file
}
