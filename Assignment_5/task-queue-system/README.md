
## Files

- `server.c`: Implements the Task Queue Server.
- `client1.c`: Implements the Worker Client thats runs normally, requests tasks,and responds back.
- `client2.c`: program to demonstrate a client that requests a task but does not respond back
- `client3.c`: program to simulate a client that requests a task repeatedly but does not complete it
- `client4.c`: program to demonstrate the client that connects to the server and does nothing
- `Makefile`: Contains build instructions for compiling the server and clients.

## Compilation

To compile the server and client, run the following command in the project directory:

```
make
```

This will generate the executables `server` and `clients`.

## Running the Server

To start the server, run the following command:

```
./server <input txtfile>
```

The server will start listening for incoming task requests.

## Running the Client

To run the client, use the following command:

```
./client{1,2,3,4}
```

The client will connect to the server and send task requests. We can run multiple instances of the client to simulate multiple workers.

