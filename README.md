# Wordle-TCP-Server

## Build Instructions

First, compile the Wordle server:
```
gcc -o wordle-server.out server_main.c wordle_server.c wordle_logic.c -pthread
```

Then, compile the client code:
```
gcc -o wordle-client.out wordle_client.c
```

## Running the Server

Run the compiled Wordle server with:
```
./wordle-server.out <port> <dictionary-filename>
```
- `<port>`: The port number for the server to listen on (e.g., 8080)
- `<dictionary-filename>`: Path to your dictionary file (see format below)

**All words in the dictionary file will be used.**

**Example:**
```
./wordle-server.out 8080 1234 wordle.txt
```

## Running the Client

To run the client (on the same or another machine):
```
./wordle-client.out
```

If you change the server port, also update this line in `wordle_client.c`:
```
unsigned short server_port = 8080;
```

## Dictionary File Format
- Plain text file
- **One 5-letter word per line**
- No spaces, punctuation, or extra characters
- Example:
```
apple
grape
peach
melon
berry
```

Have fun playing Wordle! 🎉
