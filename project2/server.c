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
#include <semaphore.h>

/* SHARED BETWEEN THREADS */
// global work queue
Queue work_queue;
// work semaphore
sem_t work_sem;
// log file
FILE *log_file;
pthread_mutex_t list_ops;
pthread_mutex_t conn_mutex;
int modified;

/* Start Server */
void run_server(int portno) {
    modified = 0;
    pthread_mutex_init(&list_ops, NULL);
    pthread_mutex_init(&conn_mutex, NULL);
    sem_init(&work_sem, 0, 0);
    /* initialise worker thread and queue */
    work_queue = NULL;
    pthread_t worker_thread;
    if(pthread_create(&worker_thread, NULL, work_thread, NULL) != 0) {
        perror("ERROR creating thread");
        exit(1);
    }
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
        pthread_mutex_lock(&conn_mutex);
        int *fd = malloc(sizeof(int));
        pthread_mutex_unlock(&conn_mutex);
        *fd = newsockfd; // prevent a race condition by loading our file descriptor into a new var
        // we have our connection, pass the descriptor to a new thread, and return to accepting connections
        pthread_t tid;
        if(pthread_create(&tid, NULL, connection_entry, fd) != 0) {
            perror("ERROR creating thread");
            exit(1);
        }
    }
}

void printMalformedError(int sockfd)
{
    write_error(sockfd, "Malformed header");
}

void connection_entry(void *arg)
{
    pthread_mutex_lock(&conn_mutex);
    int sockfd = *(int *)arg;
    free(arg);
    pthread_mutex_unlock(&conn_mutex);

    int used = 0;
    char *buff = malloc(sizeof(char) * 1024);
    bzero(buff, 1024);
    server_log(sockfd, "Connection Established", 1);
    int n;
    while((n = read(sockfd, &buff[used], 1024 - used)) > 0) { // while connection is active
        used += n;
        char *end;
        while((end = memmem(buff, used, "\r\n", 2))) {
            int len = (end - buff);
            char *line = malloc(sizeof(char) * len);
            memcpy(line, buff, len + 2);
            process(sockfd, line);
            used -= len + 2;
            memmove(buff, end + 2, used);
        }

    }
    // remove from queue
    dequeue_client(sockfd);
    server_log(sockfd, "Connection Terminated", 1);
    close(sockfd);
    pthread_exit(NULL);
}

void process(int sockfd, char *buff) {
    /* parse the header */
    if (strncmp(buff, "PING\r\n", 6) == 0) {
        server_log(sockfd, buff, 0);
        ping_handler(sockfd);
    } else if (strncmp(buff, "PONG\r\n", 6) == 0) {
        server_log(sockfd, buff, 0);
        pong_handler(sockfd);
    } else if (strncmp(buff, "OKAY\r\n", 6) == 0) {
        server_log(sockfd, buff, 0);
        okay_handler(sockfd);
    } else if (strncmp(buff, "SOLN ", 5) == 0) {
        server_log(sockfd, buff, 0);
        soln_handler(sockfd, buff + 5);
    } else if (strncmp(buff, "WORK ", 5) == 0) {
        server_log(sockfd, buff, 0);
        work_handler(sockfd, buff + 5);
    } else if (strncmp(buff, "ABRT\r\n", 6) == 0) {
        server_log(sockfd, buff, 0);
        //dequeue_client(sockfd);
        write_msg(sockfd, "OKAY", 4);
    }else {
        write_error(sockfd, "Invalid server usage");
    }
}

/* PING message handler */
void ping_handler(int sockfd)
{
    write_msg(sockfd, "PONG", 4);
}

/* PONG message handler */
void pong_handler(int sockfd)
{
    write_error(sockfd, "PONG strictly for server use");
}

/* OKAY message handler */
void okay_handler(int sockfd)
{
    write_error(sockfd, "OKAY is not okay");
}

/* SOLN message handler */
void soln_handler(int sockfd, char *buffer)
{
    /* counting vars */
    int n;

    /* parsing vars */
    char diff_stream[9]; // 8 byte hex with null byte end
    char seed_stream[65]; // 64 byte seed with null byte end
    char soln_stream[17]; // 16 byte hex with null byte end
    char *buf_counter = buffer;
    if(strnlen(buffer, 1024) != 92)
    {
        write_error(sockfd, "malformed message data");
        return;
    }

    /* read in difficulty */
    bzero(diff_stream, 9* sizeof(char));
    memcpy(diff_stream, buf_counter, 9 * sizeof(char));
    buf_counter += 9* sizeof(char);

    /* read in seed */
    bzero(seed_stream, 65* sizeof(char)); // \0 will be written to the end
    memcpy(seed_stream, buf_counter, 65 * sizeof(char));
    buf_counter += 65* sizeof(char);

    /* read in solution */
    bzero(soln_stream, 18* sizeof(char));
    memcpy(soln_stream, buf_counter, 17 * sizeof(char));
    buf_counter += 17* sizeof(char);

    if(check_proof(diff_stream, seed_stream, soln_stream) < 0) {
        write_msg(sockfd, "OKAY", 4);
    } else {
        write_error(sockfd, "invalid proof of work");
    }
}

/* WORK message handler */
void work_handler(int sockfd, char *buffer)
{
    /* counting vars */
    int n;

    /* parsing vars */
    BYTE diff_stream[9]; // 8 byte hex string with null byte end
    BYTE seed_stream[65]; // 64 byte seed with null byte end
    BYTE start_stream[17]; // 16 byte hex string with null byte end
    BYTE worker_stream[3]; // 2 byte hex string with null byte end
    char *buf_counter = buffer;
    if(strnlen(buffer, 1024) != 95)
    {
        write_error(sockfd, "malformed message data");
        return;
    }

    /* read in difficulty */
    bzero(diff_stream, 9* sizeof(char));
    memcpy(diff_stream, buf_counter, 8* sizeof(char));
    buf_counter += 9* sizeof(char);

    /* read in seed */
    bzero(seed_stream, 65* sizeof(char)); // \0 will be written to the end
    memcpy(seed_stream, buf_counter, 64* sizeof(char));
    buf_counter += 65* sizeof(char);

    /* read in start */
    bzero(start_stream, 17* sizeof(char));
    memcpy(start_stream, buf_counter, 16* sizeof(char));
    buf_counter += 17* sizeof(char);

    /* read in worker_count */
    bzero(worker_stream, 3 * sizeof(char)); // \0 will be written to the end
    memcpy(worker_stream, buf_counter, 2 * sizeof(char));
    buf_counter += 3 * sizeof(char);

    /* add to queue */
    pthread_mutex_lock(&list_ops);
    //enqueue(&work_queue, sockfd, diff_stream, seed_stream, start_stream, worker_stream);
    pthread_mutex_unlock(&list_ops);
    sem_post(&work_sem);

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
    solution = hex2int(8, soln_stream);

    bzero(concat, 40);
    // concatenate seed and solution
    memcpy(concat, seed, 32);
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

void work_thread(void *arg)
{
    /*while(1) {
        if(work_queue) {
            find_soln(work_queue->work, work_queue->sockfd);
            dequeue(&work_queue);
        } else {
            sem_wait(&work_sem);
        }
    }*/
}

void find_soln(Work work, int sockfd)
{
    uint64_t solution = strtoull(work->start, NULL, 16);
    while(1) {
        pthread_mutex_lock(&list_ops);
        if(modified) {
            modified = 0;
            pthread_mutex_unlock(&list_ops);
            break;
        }
        pthread_mutex_unlock(&list_ops);
        if(check_proof(work->difficulty, work->seed, work->start) < 0) {
            // give the message back
            char msg[98];
            bzero(msg, 98);
            sprintf(msg, "SOLN %s %s %s", work->difficulty, work->seed, work->start);
            write_msg(sockfd, msg, 95);
            break;
        }
        // increment solution
        solution++;
        sprintf(work->start, "%016llx", solution);
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
    char msg[strlen + 3 * sizeof(char)];
    sprintf(msg, "%s\r\n", str);
    n = write(sockfd, msg, strlen + 2);
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
    sprintf((char *)&msg, "[%s] %02d %-16s %s\n", time, sockfd, ip, exchange);
    fprintf(stdout, "%s", msg);
    fprintf(log_file, "%s", msg);
    fflush(log_file); // force write to file
}

void dequeue_client(int sockfd)
{
    pthread_mutex_lock(&list_ops);
    if(!work_queue) {
        pthread_mutex_unlock(&list_ops);
        return;
    }
    if(work_queue->sockfd == sockfd) {
        modified = 1;
    }
    removeWork(&work_queue, sockfd);
    pthread_mutex_unlock(&list_ops);
}
