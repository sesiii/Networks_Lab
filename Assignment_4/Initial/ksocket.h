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

// Error Codes
#define ENOSPACE -1             // No space in shared memory
#define ENOTBOUND -2            // Socket not bound
#define ENOMESSAGE -3           // No message available in buffer

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

// Declare shm as an extern variable
extern SharedMemory* shm;

// Function Prototypes
int init_shared_memory();
SharedMemory* attach_shared_memory(int shmid);
void detach_shared_memory(SharedMemory* shm);
void cleanup_shared_memory(int shmid);

// Core KTP Functions
int k_socket();
int k_bind(int sockfd, const char* src_ip, int src_port, const char* dest_ip, int dest_port);
int k_sendto(int sockfd, const void* buf, size_t len, int flags);
int k_recvfrom(int sockfd, void* buf, size_t len, int flags);
int k_close(int sockfd);

#endif // KSOCKET_H