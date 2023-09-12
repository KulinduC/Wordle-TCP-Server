# Wordle-TCP-Server

First compile the wordle server 
`gcc -o hw3-server.out hw3-main.c hw3.c -pthread`

Then compile the client code
`gcc -o hw3-client.out hw3-client.c`

Run the compiled wordle server
`./hw3-server.out 8080 <seed> <dictionary-filename> <num-words>`

To change the listener port also change the server port in hw3-client.c
`unsigned short server_port = 8080;`

Run the compiled client
`./hw3-client.out`

The server will wait on a connection from the client and a valid guess as well. Have fun playing wordle :)
