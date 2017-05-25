/* Jack Kaloger 758278
 * COMPUTER SYSTEMS PROJECT 2
 * COMP30023 2017
 * main.c
 */

#include <stdio.h>
#include "sha256.h"
#include "uint256.h"
#include "server.h"
#include <pthread.h>

#define MAX_CLIENTS 100

int
main(int argc, char **argv)
{
    if (argc < 2)
    {
        fprintf(stderr,"ERROR, no port provided\n");
        exit(1);
    }

    run_server(atoi(argv[1]));
    return 0;
}
