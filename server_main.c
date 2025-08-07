#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "client_handler.h"
#include "wordle_server.h"

/* Global variable definitions */
int total_guesses;
int total_wins;
int total_losses;

int main( int argc, char ** argv )
{
  total_guesses = total_wins = total_losses = 0;

  int rc = wordle_server( argc, argv );

  return rc;
}