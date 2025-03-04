#ifndef KSOCKET_H
#define KSOCKET_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <errno.h>
#include <stdbool.h>

#define SOCK_KTP 1000
#define T 5
#define P 0.1
#define MAX_SEQ_NUM 256 // 8-bit sequence number, so max is 256
#define TIMEOUT 2  // Timeout in seconds
#define MAX_RETRANSMISSIONS 5

extern int errno;

int k_socket(int domain, int type, int protocol);
int k_bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
ssize_t k_sendto(int sockfd, const void *buf, size_t len, int flags,
                 const struct sockaddr *dest_addr, socklen_t addrlen);
ssize_t k_recvfrom(int sockfd, void *buf, size_t len, int flags,
                   struct sockaddr *src_addr, socklen_t *addrlen);
int k_close(int sockfd);
int dropMessage(float p);

#endif // KSOCKET_H