#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
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

    char buffer[2048];
    struct sockaddr_in src;
    socklen_t len = sizeof(src);
    int bytes = recvfrom(sockfd, buffer, sizeof(buffer), 0,
                         (struct sockaddr *)&src, &len);
    struct iphdr *ip = (struct iphdr *)buffer;
    char *payload = buffer + ip->ihl * 4;
    printf("Received: %s\n", payload);
    return 0;
}