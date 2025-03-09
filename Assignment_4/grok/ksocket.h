// #ifndef KSOCKET_H
// #define KSOCKET_H

// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <unistd.h>
// #include <sys/socket.h>
// #include <netinet/in.h>
// #include <arpa/inet.h>
// #include <pthread.h>
// #include <sys/ipc.h>
// #include <sys/shm.h>
// #include <time.h>

// #define MAX_SOCKETS 2           // Number of KTP sockets
// #define MESSAGE_SIZE 512        // Payload size
// #define WINDOW_SIZE 4           // Sliding window size
// #define PORT_USER1 12345
// #define PORT_USER2 54321

// // Error Codes
// #define ENOSPACE -1
// #define ENOTBOUND -2
// #define ENOMESSAGE -3

// // Message Types
// #define MSG_TYPE_DATA 0
// #define MSG_TYPE_ACK 1

// // KTP Message Structure
// typedef struct {
//     uint8_t seq_num;          // 8-bit sequence number
//     uint8_t type;             // 0 for data, 1 for ACK
//     char payload[MESSAGE_SIZE]; // Data or rwnd size (as string for ACK)
// } KTP_Message;

// // Sender Window Structure
// typedef struct {
//     KTP_Message messages[WINDOW_SIZE];
//     int base;                 // Oldest unacked sequence number
//     int next_seq;             // Next sequence number to send
//     time_t timestamps[WINDOW_SIZE]; // Timestamps for retransmission
//     int size;                 // Number of unacked messages
// } SenderWindow;

// // Receiver Window Structure
// typedef struct {
//     KTP_Message messages[WINDOW_SIZE];
//     int base;                 // Next expected sequence number
//     int size;                 // Number of buffered messages
//     int rwnd;                 // Receiver window size (available slots)
// } ReceiverWindow;

// // KTP Socket Structure
// typedef struct {
//     int is_free;              // 1 if free, 0 if allotted
//     pid_t process_id;         // Process ID of the creator
//     int sock_id;              // UDP socket FD (managed by thread_r)
//     struct sockaddr_in src_addr;  // Source IP/Port
//     struct sockaddr_in dest_addr; // Destination IP/Port
//     KTP_Message send_buffer[WINDOW_SIZE]; // Buffer for outgoing messages
//     KTP_Message recv_buffer[WINDOW_SIZE]; // Buffer for incoming messages
//     int send_count;           // Number of messages in send_buffer
//     int recv_count;           // Number of messages in recv_buffer
//     SenderWindow swnd;        // Sender window
//     ReceiverWindow rwnd;      // Receiver window
// } KTP_Socket;

// // Shared Memory Structure
// typedef struct {
//     KTP_Socket sockets[MAX_SOCKETS];
// } SharedMemory;

// extern SharedMemory* shm;

// int init_shared_memory();
// SharedMemory* attach_shared_memory(int shmid);
// void detach_shared_memory(SharedMemory* shm);
// void cleanup_shared_memory(int shmid);

// int k_socket();
// int k_bind(int sockfd, const char* src_ip, int src_port, const char* dest_ip, int dest_port);
// int k_sendto(int sockfd, const void* buf, size_t len, int flags);
// int k_recvfrom(int sockfd, void* buf, size_t len, int flags);
// int k_close(int sockfd);

// void* thread_r(void* arg);
// void* thread_s(void* arg);

// typedef struct {
//     int shmid;
// } ThreadArgs;

// #endif // KSOCKET_H



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
#include <time.h>

#define MAX_SOCKETS 2           // Number of KTP sockets
#define MESSAGE_SIZE 512        // Payload size
#define WINDOW_SIZE 3        // Increased sliding window size
#define PORT_USER1 12345
#define PORT_USER2 54321
#define DROP_PROBABILITY 0.1    // 10% packet loss probability

// Error Codes
#define ENOSPACE -1
#define ENOTBOUND -2
#define ENOMESSAGE -3

// Message Types
#define MSG_TYPE_DATA 0
#define MSG_TYPE_ACK 1

// KTP Message Structure
typedef struct {
    uint8_t seq_num;          // 8-bit sequence number
    uint8_t type;             // 0 for data, 1 for ACK
    char payload[MESSAGE_SIZE]; // Data or rwnd size (as string for ACK)
} KTP_Message;

// Sender Window Structure
typedef struct {
    KTP_Message messages[WINDOW_SIZE];
    int base;                 // Oldest unacked sequence number
    int next_seq;             // Next sequence number to send
    time_t timestamps[WINDOW_SIZE]; // Timestamps for retransmission
    int size;                 // Number of unacked messages
} SenderWindow;

// Receiver Window Structure
typedef struct {
    KTP_Message messages[WINDOW_SIZE];
    int base;                 // Next expected sequence number
    int size;                 // Number of buffered messages
    int rwnd;                 // Receiver window size (available slots)
} ReceiverWindow;

// KTP Socket Structure
typedef struct {
    int is_free;              // 1 if free, 0 if allotted
    pid_t process_id;         // Process ID of the creator
    int sock_id;              // UDP socket FD (managed by thread_r)
    struct sockaddr_in src_addr;  // Source IP/Port
    struct sockaddr_in dest_addr; // Destination IP/Port
    KTP_Message send_buffer[WINDOW_SIZE]; // Buffer for outgoing messages
    KTP_Message recv_buffer[WINDOW_SIZE]; // Buffer for incoming messages
    int send_count;           // Number of messages in send_buffer
    int recv_count;           // Number of messages in recv_buffer
    SenderWindow swnd;        // Sender window
    ReceiverWindow rwnd;      // Receiver window
} KTP_Socket;

// Shared Memory Structure
typedef struct {
    KTP_Socket sockets[MAX_SOCKETS];
} SharedMemory;

extern SharedMemory* shm;

int init_shared_memory();
SharedMemory* attach_shared_memory(int shmid);
void detach_shared_memory(SharedMemory* shm);
void cleanup_shared_memory(int shmid);

int k_socket();
int k_bind(int sockfd, const char* src_ip, int src_port, const char* dest_ip, int dest_port);
int k_sendto(int sockfd, const void* buf, size_t len, int flags);
int k_recvfrom(int sockfd, void* buf, size_t len, int flags);
int k_close(int sockfd);

void* thread_r(void* arg);
void* thread_s(void* arg);
void* thread_g(void* arg); // Garbage collector thread

typedef struct {
    int shmid;
} ThreadArgs;

int dropMessage(); // Simulate packet loss

#endif // KSOCKET_H