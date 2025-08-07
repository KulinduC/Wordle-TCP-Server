#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include "client_handler.h"
#include "wordle_logic.h"


int wordle_server( int argc, char ** argv ) {

    pthread_mutex_init(&thread_mutex, NULL);

    signal(SIGUSR1, SIGEND_WORDLE);
    signal(SIGINT, SIG_IGN);
    signal(SIGTERM, SIG_IGN);
    signal(SIGUSR2, SIG_IGN);

    setvbuf( stdout, NULL, _IONBF, 0 );

    if (argc != 5) {
        fprintf(stderr, "ERROR: Invalid argument(s)\n");
        fprintf(stderr, "USAGE: hw3.out <listener-port> <seed> <dictionary-filename> <num-words>\n");
        return EXIT_FAILURE;
    }

    int port, seed;
    char* fn;

    port = atoi(*(argv + 1));
    seed = atoi(*(argv + 2));
    fn = *(argv + 3);
    num_words = atoi(*(argv + 4));

    words = wordsList(fn, num_words);
    srand(seed);


    if (words == NULL) {
        fprintf(stderr, "ERROR: Word file unreadable\n");
        return EXIT_FAILURE;
    }

    int listener_sd = socket( AF_INET, SOCK_STREAM, 0 );
    if (listener_sd == -1) {
        perror("socket() failed"); 
        return EXIT_FAILURE;
    }

    struct sockaddr_in tcp_server;
    tcp_server.sin_family = AF_INET; 
    tcp_server.sin_port = htons((unsigned short) port);
    tcp_server.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(listener_sd, (struct sockaddr*)&tcp_server, sizeof(tcp_server)) == -1) {
        perror("bind() failed");
        return EXIT_FAILURE;
    }

    if (listen(listener_sd, 32) == -1) {
        perror("listen() failed");
        return EXIT_FAILURE;
    }

    printf(" opened %s (%d words)\n", fn, num_words);
    printf(" seeded pseudo-random number generator with %d\n", seed);
    printf(" Wordle server listening on port \n");


    while(1) {
        struct sockaddr_in rc;
        int len = sizeof(rc);
        fflush(stdout);
        int sd = accept(listener_sd, (struct sockaddr *)&rc,(socklen_t *)&len);
        if (sd == -1) {
            perror("accept() failed");
            continue;
        }
        printf(" rcvd incoming connection request\n");

        Client *cd = (Client *)calloc(1, sizeof(Client));
        cd->sd = sd;
        cd->index = rand() % num_words;

        pthread_t ct;
        pthread_create(&ct, NULL, game, cd);
        pthread_detach(ct);
    }
    close(listener_sd);
    printf("MAIN: valid guesses: %d\n",total_guesses);
    printf("MAIN: win/loss: %d/%d\n",total_wins, total_losses);
     

    return EXIT_SUCCESS;
}