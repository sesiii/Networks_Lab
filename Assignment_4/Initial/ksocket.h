#ifndef KSOCKET_H
#define KSOCKET_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define MAX_SOCKETS 10          // Maximum number of KTP sockets
#define MESSAGE_SIZE 512        // Fixed message size (512 bytes)
#define WINDOW_SIZE 10          // Window size for flow control
#define SEQ_NUM_BITS 8          // Sequence number is 8 bits (0-255)

// KTP Socket Structure
typedef struct {
    int is_free;                // 1 if free, 0 if allotted
    pid_t process_id;           // Process ID of the creator
    int udp_socket;             // Corresponding UDP socket
    struct sockaddr_in src_addr; // Source IP/Port
    struct sockaddr_in dest_addr; // Destination IP/Port
    char send_buffer[WINDOW_SIZE][MESSAGE_SIZE]; // Send buffer
    char recv_buffer[WINDOW_SIZE][MESSAGE_SIZE]; // Receive buffer
    int swnd_size;              // Sender window size
    int rwnd_size;              // Receiver window size
    int last_ack;               // Last acknowledged sequence number
    int last_seq_sent;          // Last sequence number sent
    int last_seq_recv;          // Last sequence number received
} KTP_Socket;

// Shared Memory Structure
typedef struct {
    KTP_Socket sockets[MAX_SOCKETS]; // Array of KTP sockets
} SharedMemory;

// Data Message Format
typedef struct {
    uint8_t seq_num;            // Sequence number (8 bits)
    char payload[MESSAGE_SIZE]; // Payload (512 bytes)
} DataMessage;

// ACK Message Format
typedef struct {
    uint8_t ack_num;            // Last in-order sequence number received
    int rwnd_size;              // Updated receiver window size
} AckMessage;

// Function Prototypes
int init_shared_memory();
SharedMemory* attach_shared_memory(int shmid);
void detach_shared_memory(SharedMemory* shm);
void cleanup_shared_memory(int shmid);

#endif // KSOCKET_H