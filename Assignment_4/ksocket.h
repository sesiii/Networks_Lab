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

#define MAX_SOCKETS 2
#define MESSAGE_SIZE 512
#define WINDOW_SIZE 3
#define PORT_USER1 12345
#define PORT_USER2 54321
#define T 5          
#define P 0.1        

#define ENOSPACE -1
#define ENOTBOUND -2
#define ENOMESSAGE -3

#define MSG_TYPE_DATA 0
#define MSG_TYPE_ACK 1

typedef struct {
    uint8_t seq_num;
    uint8_t type;
    char payload[MESSAGE_SIZE];
} KTP_Message;

typedef struct {
    KTP_Message messages[WINDOW_SIZE];
    int base;
    int next_seq;
    time_t timestamps[WINDOW_SIZE];
    int size;
} SenderWindow;

typedef struct {
    KTP_Message messages[WINDOW_SIZE];
    int base;
    int size;
    int rwnd;
} ReceiverWindow;

typedef struct {
    int is_free;
    pid_t process_id;
    int sock_id;
    struct sockaddr_in src_addr;
    struct sockaddr_in dest_addr;
    KTP_Message send_buffer[WINDOW_SIZE];
    KTP_Message recv_buffer[WINDOW_SIZE];
    int send_count;
    int recv_count;
    SenderWindow swnd;
    ReceiverWindow rwnd;
} KTP_Socket;

typedef struct {
    KTP_Socket sockets[MAX_SOCKETS];
    float drop_probability;
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
void* thread_g(void* arg);

typedef struct {
    int shmid;
} ThreadArgs;

int dropMessage(float probability);

#endif // KSOCKET_H
