// client_handler.h
#ifndef CLIENT_HANDLER_H
#define CLIENT_HANDLER_H

#include <pthread.h>

typedef struct data {
    int sd;
    int index;
} Client;

extern int total_guesses;
extern int total_wins;
extern int total_losses;
extern char **words;
extern int num_words;
extern pthread_mutex_t thread_mutex;

#endif