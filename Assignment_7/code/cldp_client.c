#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include<time.h>
#include <netinet/ip.h>
#include <netinet/in.h>

// Define your protocol number
#define CLDP_PROTOCOL 253

// Define your CLDP header structure
struct cldp_header {
    uint8_t type;        // Message type (e.g., HELLO = 0x01)
    uint8_t payload_len; // Length of payload
    uint16_t trans_id;   // Transaction ID
    uint32_t reserved;   // Reserved bytes
};

// Simple checksum function for IP header
unsigned short ip_checksum(void *buf, int len) {
    // Skeleton: Add checksum logic here
    return 0; // Placeholder
}

int main() {
    // Create raw socket
    int sock = socket(AF_INET, SOCK_RAW, CLDP_PROTOCOL);
    if (sock < 0) {
        perror("Socket creation failed");
        return 1;
    }

    // Enable IP header inclusion
    int one = 1;
    if (setsockopt(sock, IPPROTO_IP, IP_HDRINCL, &one, sizeof(one)) < 0) {
        perror("setsockopt failed");
        return 1;
    }

    // Prepare HELLO packet
    char packet[512];
    struct iphdr *ip = (struct iphdr*)packet;
    struct cldp_header *cldp = (struct cldp_header*)(packet + sizeof(struct iphdr));

    // Fill IP header (skeleton)
    ip->version = 4;
    ip->ihl = 5;
    ip->protocol = CLDP_PROTOCOL;
    ip->saddr = inet_addr("192.168.1.101"); // Your IP
    ip->daddr = inet_addr("255.255.255.255"); // Broadcast

    // Fill CLDP header for HELLO
    cldp->type = 0x01;
    char *payload = (char*)(cldp + 1);
    strcpy(payload, "Hi");
    cldp->payload_len = strlen("Hi");

    // Set total length and checksum (skeleton)
    ip->tot_len = htons(sizeof(struct iphdr) + sizeof(struct cldp_header) + cldp->payload_len);
    ip->check = ip_checksum(ip, sizeof(struct iphdr));

    // Send HELLO
    struct sockaddr_in dest = {.sin_family = AF_INET, .sin_addr.s_addr = INADDR_BROADCAST};
    sendto(sock, packet, ntohs(ip->tot_len), 0, 
           (struct sockaddr*)&dest, sizeof(dest));

    // Receive HELLO messages
    char resp_buf[512];
    while (1) {
        int len = recv(sock, resp_buf, sizeof(resp_buf), 0);
        if (len < 0) continue;

        struct iphdr *resp_ip = (struct iphdr*)resp_buf;
        if (resp_ip->protocol != CLDP_PROTOCOL) continue;

        struct cldp_header *resp_cldp = (struct cldp_header*)(resp_buf + sizeof(struct iphdr));
        if (resp_cldp->type == 0x01) {
            char *resp_payload = (char*)(resp_cldp + 1);
            printf("Received HELLO from %s: %.*s\n", 
                   inet_ntoa(*(struct in_addr*)&resp_ip->saddr), 
                   resp_cldp->payload_len, resp_payload);
        }
    }

    close(sock);
    return 0;
}