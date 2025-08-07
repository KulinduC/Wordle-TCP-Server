#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <pthread.h>
#include <ctype.h>
#include <signal.h>
#include "client_handler.h"

int count_lines(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) return 0;
    int count = 0;
    char buffer[128];
    while (fgets(buffer, sizeof(buffer), file)) {
        count++;
    }
    fclose(file);
    return count;
}

void free_mem(char **words, int num) { //free memory function
    if (words != NULL) {
        for (int i = 0; i < num; i++) {
            free(*(words + i));
        }
        free(words);
    }
}

int found(char **list, char *word, int num) { //found word function
    for(int i = 0; i < num; i++){
        if (strcmp(word, *(list+i)) == 0) {
            return 1; //true 
        }
    }
    return 0; //false
}


void check(char *guess, char *ans){ //sending string function
    for (int i = 0; i < 5; i++){
        if(*(guess + i) == *(ans + i)){
            *(guess + i) = toupper(*(ans + i)); 
            continue;
        }
        int blank = 0;
        for(int j = 0; j < 5; j++){ //checking repeating letters 
            char rep = tolower(*(guess + j));
            if(rep != *(ans + j) && *(guess + i) == *(ans + j)) { 
                blank += 1;
            }
        }
        if(blank == 0) *(guess + i) = '-'; //no matching character
    }
}

char **wordsList(const char *fn, int num) { //getting the wordList
    FILE *file = fopen(fn, "r");
    if (file == NULL) {
        perror("Error opening file");
        return NULL;
    }

    char **wordList = (char**)calloc(num + 1, sizeof(char *));
    if (wordList == NULL) {
        perror("Memory alloc failed");
        fclose(file);
        return NULL;
    }

    for (int i = 0; i < num; i++) {
        char* word = (char*)calloc(6, sizeof(char));
        if (fscanf(file, "%s", word) != 1) {
            free(word);
            perror("Error reading word");
            return NULL;
        }

        *(word+5) = '\0';

        int len = strlen(word);
        *(wordList + i) = (char *)calloc(len + 1, sizeof(char));
        if (*(wordList + i) == NULL) {
            perror("Memory allocation error");
            return NULL;
        }

        for (int j = 0; j < len; j++) {
            *(word + j) = tolower(*(word + j));
        }
        strcpy(*(wordList + i), word);
        free(word);
    }
    
    fclose(file);
    return wordList;
}



void SIGEND_WORDLE(int signum) {
    printf(" SIGUSR1 rcvd; Wordle server shutting down...\n");
    printf("MAIN: valid guesses: %d\n",total_guesses);
    printf("MAIN: win/loss: %d/%d\n",total_wins, total_losses);
    
    // Free the words list before exiting
    if (words != NULL) {
        free_mem(words, num_words);
    }
    
    exit(1);
}

pthread_mutex_t thread_mutex;

void *game(void *arg) {

    short i, clientBytes;
    char* buffer = (char*)calloc(32 , sizeof(char)); // Buffer to store received data
    char* response = (char*)calloc(8 , sizeof(char)); //8 byte response


    Client *cd2 = (Client *)arg;
    int sd = cd2->sd;
    int pos = cd2->index;
    char* word = *(words + pos);

    int won = 0; //lose if 0, win if 1 
    for (i = 5; i >= 0; i--) {
        printf(" waiting for guess\n");
        int bytes = recv(sd, buffer, sizeof(buffer), 0);
        if (bytes == 0){
            printf(" client gave up; closing TCP connection...\n");
            break;
        }
        printf("THREAD %p: rcvd guess: %s\n", (void*)pthread_self(),buffer);
        for (int j = 0; j < bytes; j++) {
            *(buffer + j) = tolower(*(buffer + j));
        }

        if ((bytes != 5) || (!found(words, buffer, num_words))) {
            i++;
            clientBytes = htons(i);
            *(response) = 'N';
            memcpy(response + 1, &clientBytes, sizeof(short));
            memcpy(response + 3, "?????", 5);

            printf("THREAD %p: invalid guess; sending reply: ????? (%d guess%s left)\n",
            (void*)pthread_self(), i, i == 1 ? "" : "es");

            
            send(sd, response, 8, 0);
            continue;
        }

        check(buffer, word);
        *(buffer + 5) = '\0';
        clientBytes = htons(i);

        *(response) = 'Y';
        memcpy(response + 1, &clientBytes, sizeof(short));
        memcpy(response + 3, buffer, 5);


        pthread_mutex_lock(&thread_mutex);
        total_guesses++;
        pthread_mutex_unlock(&thread_mutex);

        printf("THREAD %p: sending reply: %s (%d guess%s left)\n",
        (void*)pthread_self(), buffer, i, i == 1 ? "" : "es");
        
        send(sd, response, 8, 0);

        for (int i = 0; i < 5; ++i) *(buffer + i) = tolower(*(buffer + i));
        if (strcmp(buffer, word) == 0) {
            won = 1; 
            break;
        }
    }
    for(int i = 0; i < 5; i++) *(word + i) = toupper(*(word + i));
    if (won == 0) {
        pthread_mutex_lock(&thread_mutex);
        total_losses++;
        pthread_mutex_unlock(&thread_mutex);
    }
    else {
        pthread_mutex_lock(&thread_mutex);
        total_wins++;
        pthread_mutex_unlock(&thread_mutex);
    }
    printf(" game over; word was %s!\n", word);

    close(sd);
    free(cd2); 
    free(response);
    free(buffer);

    return NULL;
}