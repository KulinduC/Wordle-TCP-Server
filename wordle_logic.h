// wordle_logic.h
#ifndef WORDLE_LOGIC_H
#define WORDLE_LOGIC_H


void free_mem(char **words, int num);
void check(char *guess, char *ans);
int found(char **list, char *word, int num);
char **wordsList(const char *fn, int num);
void SIGEND_WORDLE(int signum);
void *game(void *arg);
int count_lines(const char *filename);

#endif