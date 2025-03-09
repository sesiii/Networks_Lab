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
#include <errno.h>

#define MAX_SOCKETS 10 // Maximum number of KTP sockets
#define MESSAGE_SIZE 512

// Error codes / special return values
#define ENOSPACE   -1
#define ENOTBOUND  -2
#define ENOMESSAGE -3

// Simple KTP socket structure
typedef struct {
    int is_free;      // 1 if free, 0 if allocated
    int udp_fd;       // Underlying UDP file descriptor
    pid_t creator_pid;// Creator process ID

    struct sockaddr_in src_addr; // local (bind) address
    struct sockaddr_in dst_addr; // remote address

    // Basic buffers for storing a single message
    char recv_buffer[MESSAGE_SIZE];
    char send_buffer[MESSAGE_SIZE];
    int has_data;
} KTP_Socket;

// Shared memory structure
typedef struct {
    KTP_Socket sockets[MAX_SOCKETS];
} SharedMemory;

// Extern reference
extern SharedMemory* shm;

// Shared memory functions
int init_shared_memory();
SharedMemory* attach_shared_memory(int shmid);
void detach_shared_memory(SharedMemory* ptr);
void cleanup_shared_memory(int shmid);

// Core KTP calls
int k_socket();
int k_bind(int sockfd, const char* src_ip, int src_port, const char* dst_ip, int dst_port);
int k_sendto(int sockfd, const void* buf, size_t len, int flags);
int k_recvfrom(int sockfd, void* buf, size_t len, int flags);
int k_close(int sockfd);

// Thread receiving
void* thread_r(void* arg);

// Thread R arg structure
typedef struct {
    int shmid;
} ThreadRArgs;

#endif