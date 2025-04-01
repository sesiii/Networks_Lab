#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#define PORT 8080
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 10

// Raw socket server (no client needed as it’s low-level)
struct ip_header {
    unsigned char version_ihl;
    unsigned char tos;
    unsigned short total_len;
    unsigned short id;
    unsigned short frag_off;
    unsigned char ttl;
    unsigned char protocol;
    unsigned short checksum;
    unsigned int src_addr;
    unsigned int dst_addr;
} __attribute__((packed));

int main() {
    int sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
    if (sockfd < 0) {
        perror("Raw socket failed");
        return 1;
    }

    char packet[BUFFER_SIZE];
    struct ip_header *iph = (struct ip_header*)packet;

    iph->version_ihl = (4 << 4) | 5;
    iph->tos = 0;
    iph->total_len = htons(sizeof(struct ip_header) + 20);
    iph->id = htons(12345);
    iph->frag_off = 0;
    iph->ttl = 64;
    iph->protocol = 253;
    iph->checksum = 0;
    iph->src_addr = inet_addr("192.168.1.1");
    iph->dst_addr = inet_addr("192.168.1.2");

    strcpy(packet + sizeof(struct ip_header), "Custom Data");

    struct sockaddr_in dest;
    dest.sin_family = AF_INET;
    dest.sin_addr.s_addr = iph->dst_addr;

    if (sendto(sockfd, packet, ntohs(iph->total_len), 0, (struct sockaddr*)&dest, sizeof(dest)) < 0) {
        perror("Send failed");
    } else {
        printf("Raw packet sent!\n");
    }
    close(sockfd);
    return 0;
}