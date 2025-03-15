
## Files

- `server.c`: Implements the Task Queue Server.
- `client.c`: Implements the Worker Client.
- `Makefile`: Contains build instructions for compiling the server and client.

## Compilation

To compile the server and client, run the following command in the project directory:

```
make
```

This will generate the executables `server` and `client`.

## Running the Server

To start the server, run the following command:

```
./server <input txtfile>
```

The server will start listening for incoming task requests.

## Running the Client

To run the client, use the following command:

```
./client
```

The client will connect to the server and send task requests. You can run multiple instances of the client to simulate multiple workers.

## Functionality

- The server can handle multiple clients in a loop.
- The client can send multiple task requests in a single run.
- The server processes each task and sends back the results to the client.


