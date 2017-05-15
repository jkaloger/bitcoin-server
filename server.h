/* Jack Kaloger 758278
 * COMPUTER SYSTEMS PROJECT 2
 * COMP30023 2017
 * server.h
 */

#ifndef SERVER_H
#define SERVER_H

void runServer(int portno);
void printMalformedError(int fd);
void entry_point(int *arg);
void ping_handler(int sockfd);
void pong_handler(int sockfd);
void okay_handler(int sockfd);
void soln_handler(int sockfd);
void work_handler(int sockfd);

#endif
