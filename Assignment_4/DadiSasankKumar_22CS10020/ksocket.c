//Name: Dadi Sasank Kumar
//Roll: 22CS10020
//Assignment 4
//ksocket.c
#include "ksocket.h"

int k_socket(int domain, int type, int protocol) {
    if (type != SOCK_KTP) {
        errno = EINVAL;
        return -1;
    }
    // Create a UDP socket
    int sockfd = socket(domain, SOCK_DGRAM, protocol);
    if (sockfd < 0) {
        return -1;
    }
    return sockfd;
}

int k_bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen) {
    return bind(sockfd, addr, addrlen);
}

ssize_t k_sendto(int sockfd, const void *buf, size_t len, int flags,
                 const struct sockaddr *dest_addr, socklen_t addrlen) {
    // Simulate message drop
    if (dropMessage(P)) {
        return len; // Pretend message was sent
    }
    return sendto(sockfd, buf, len, flags, dest_addr, addrlen);
}

ssize_t k_recvfrom(int sockfd, void *buf, size_t len, int flags,
                   struct sockaddr *src_addr, socklen_t *addrlen) {
    return recvfrom(sockfd, buf, len, flags, src_addr, addrlen);
}

int k_close(int sockfd) {
    return close(sockfd);
}

int dropMessage(float p) {
    float r = (float)rand() / RAND_MAX;
    return r < p;
}