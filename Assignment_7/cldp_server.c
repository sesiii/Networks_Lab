#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define port 8080

int main()
{
    int sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sockfd < 0)
    {
        printf("Socket creation failed\n");
        exit(0);
    }
    else
    {
        printf("Socket created successfully\n");
    }

    int opt = 1;
    setsockopt(sockfd, IPPROTO_IP, IP_HDRINCL, &opt, sizeof(opt));
    // struct iphdr
    char packet[4096];
    memset(packet, 0, 4096);

    struct iphdr *ip = (struct iphdr *)packet;
    strcpy(packet + sizeof(struct iphdr), "Hello!");
    ip->version = 4;
    ip->ihl = 5;
    ip->ttl = 64;
    ip->protocol = 253; // Custom protocol
    ip->saddr = inet_addr("192.168.1.10");
    ip->daddr = inet_addr("192.168.1.20");

    struct sockaddr_in saddr, caddr;

    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(port);
    saddr.sin_addr.s_addr = INADDR_ANY;

    sendto(sock, packet, sizeof(struct iphdr) + strlen("Hello!"), 0,
           (struct sockaddr *)&dst, sizeof(dst));
}