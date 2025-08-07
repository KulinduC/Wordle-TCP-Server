#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>

int main()
{
  /* create TCP client socket (endpoint) */
  int sd = socket(AF_INET, SOCK_STREAM, 0);
  if (sd == -1) { perror("socket() failed"); exit( EXIT_FAILURE ); }

  //struct hostent * hp = gethostbyname( "linux02.cs.rpi.edu" );


  //struct hostent * hp = gethostbyname( "128.113.126.39" );
  struct hostent * hp = gethostbyname("localhost");
  //struct hostent * hp = gethostbyname( "127.0.0.1" );


  /* TO DO: rewrite the code above to use getaddrinfo() */

  if ( hp == NULL )
  {
    fprintf(stderr, "ERROR: gethostbyname() failed\n");
    return EXIT_FAILURE;
  }

  struct sockaddr_in tcp_server;
  tcp_server.sin_family = AF_INET;  /* IPv4 */
  memcpy( (void *)&tcp_server.sin_addr, (void *)hp->h_addr, hp->h_length );
  unsigned short server_port = 8080;
  tcp_server.sin_port = htons( server_port );

  printf("CLIENT: TCP server address is %s\n", inet_ntoa(tcp_server.sin_addr));

  printf("CLIENT: connecting to server...\n");

  if (connect(sd,(struct sockaddr*)&tcp_server, sizeof(tcp_server)) == -1)
  {
    perror("CLIENT: connect() failed\n");
    return EXIT_FAILURE;
  }

  printf("CLIENT: Enter a 5-letter word\n");

  /* The implementation of the application protocol is below... */

while (1)
{
  char buffer[8];
  if (fgets(buffer, 8, stdin) == NULL) break; // reads up to 7 characters, stores at most 5 letters + \n + \0

  buffer[strcspn(buffer, "\n")] = '\0'; /* get rid of the '\n' */
  if (strlen(buffer) != 5 ) { printf("CLIENT: invalid -- try again\n"); continue; }

  printf("CLIENT: Sending to server: %s\n", buffer);
  int n = write(sd, buffer, strlen(buffer));    /* or use send()/recv() */
  if (n == -1) { perror( "write() failed" ); return EXIT_FAILURE; }

  n = read(sd, buffer, 8);    /* BLOCKING */

  if (n == -1)
  {
    perror("CLIENT: read() failed");
    return EXIT_FAILURE;
  }
  else if (n == 0)
  {
    printf("CLIENT: rcvd no data; TCP server socket was closed\n");
    break;
  }
  else
  {
    switch (buffer[0])
    {
      case 'N': printf("CLIENT: invalid guess -- try again"); break;
      case 'Y': printf("CLIENT: response: %s", buffer + 3); break;
      default: break;
    }

    short guesses = ntohs(*(short *)(buffer + 1));
    printf(" -- %d guess%s remaining\n", guesses, guesses == 1 ? "" : "es");
    if ( guesses == 0 ) break;
  }
}

  printf("CLIENT: disconnecting...\n");

  close(sd);

  return EXIT_SUCCESS;
}