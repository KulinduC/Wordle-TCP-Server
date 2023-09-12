#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <pthread.h>
#include <ctype.h>
#include <signal.h>

typedef struct data {
    int sd;
    int num;
    int index;
    char ** wordList;
} Client;

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

    char **words = (char**)calloc(num + 1, sizeof(char *));
    if (words == NULL) {
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
        *(words + i) = (char *)calloc(len + 1, sizeof(char));
        if (*(words + i) == NULL) {
            perror("Memory allocation error");
            return NULL;
        }

        for (int j = 0; j < len; j++) {
            *(word + j) = tolower(*(word + j));
        }
        strcpy(*(words + i), word);
        free(word);
    }
    
    fclose(file);
    return words;
}


pthread_mutex_t thread_mutex;


extern int total_guesses;
extern int total_wins;
extern int total_losses;
extern char ** words;

void SIGEND_WORDLE(int signum) {
    printf(" SIGUSR1 rcvd; Wordle server shutting down...\n");
    printf("MAIN: valid guesses: %d\n",total_guesses);
    printf("MAIN: win/loss: %d/%d\n",total_wins, total_losses);
    exit(1);
}

void *game(void *arg) {

    short i, clientBytes;
    char* buffer = (char*)calloc(32 , sizeof(char)); // Buffer to store received data
    char* response = (char*)calloc(8 , sizeof(char)); //8 byte response


    Client *cd2 = (Client *)arg;
    int sd = cd2->sd;
    int num_words = cd2->num;
    int pos = cd2->index;
    char** words = cd2->wordList;
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


            if (i == 1) {
                printf("THREAD %p: invalid guess; sending reply: ????? (%d guess left)\n", (void*)pthread_self(), i);
            }
            else{
                printf("THREAD %p: invalid guess; sending reply: ????? (%d guesses left)\n", (void*)pthread_self(),i);
            }
            
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


        if (i == 1) {
            printf("THREAD %p: sending reply: %s (%d guess left)\n", (void*)pthread_self(),buffer,i);
        }
        else{
            printf("THREAD %p: sending reply: %s (%d guesses left)\n", (void*)pthread_self(),buffer,i);
        }
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


int wordle_server( int argc, char ** argv ) {
    total_guesses = total_wins = total_losses = 0;
    words = (char**)calloc( 1, sizeof( char * ) );
    *words = NULL;

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

    int port, seed, num_words;
    char* fn;
    char** words;

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
        cd->num = num_words;
        cd->index = rand() % num_words;
        cd->wordList = words;

        pthread_t ct;
        pthread_create(&ct, NULL, game, cd);
        pthread_detach(ct);
    }
    close(listener_sd);
    printf("MAIN: valid guesses: %d\n",total_guesses);
    printf("MAIN: win/loss: %d/%d\n",total_wins, total_losses);
     

    return EXIT_SUCCESS;
}