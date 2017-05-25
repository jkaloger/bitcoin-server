/* Jack Kaloger 758278
 * COMPUTER SYSTEMS PROJECT 2
 * COMP30023 2017
 * server.h
 */

#ifndef SERVER_H
#define SERVER_H

#ifndef BYTE_TYPEDEF

#define BYTE_TYPEDEF

typedef unsigned char BYTE;

#endif

void runServer(int portno);
void server_loop(int sockfd);
void printMalformedError(int fd);
void entry_point(void *arg);
void ping_handler(int sockfd);
void pong_handler(int sockfd);
void okay_handler(int sockfd);
void soln_handler(int sockfd);
void work_handler(int sockfd);
void line_end_check(int sockfd);
void write_error(int sockfd, char *str);
void server_log(int sockfd, char *exchange, int is_server);
BYTE *hex2int(int num_bytes, char *bytestream);
#endif
