#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include<time.h>
#include <netinet/in.h>

#define protocol_number 253

struct cldp_header {
    uint8_t type;        
    uint8_t payload_len; 
    uint16_t trans_id;  
    uint32_t reserved;   
    uint8_t source_ip;
    uint8_t dest_ip;
    uint8_t source_port;
    uint8_t dest_port;
    uint8_t checksum;
    uint8_t hostname;

    // layout:
    // header | hostname | payload
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

    printf("Socket created\n");
    // Enable IP header inclusion
    int one = 1;
    if (setsockopt(sock, IPPROTO_IP, IP_HDRINCL, &one, sizeof(one)) < 0) {
        perror("setsockopt failed");
        return 1;
    }
    printf("IP header inclusion enabled\n");
    char buffer[512]; // Buffer for sending/receiving packets
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);

    while (1) {
        // Receive packets
        int len = recvfrom(sock, buffer, sizeof(buffer), 0, 
                          (struct sockaddr*)&addr, &addr_len);
        if (len < 0) continue;

        // Extract IP header
        struct iphdr *ip = (struct iphdr*)buffer;
        if (ip->protocol != CLDP_PROTOCOL) continue;

        // Extract CLDP header
        struct cldp_header *cldp = (struct cldp_header*)(buffer + sizeof(struct iphdr));

        // Check for HELLO message (type 0x01)
        if (cldp->type == 0x01) {
            char *payload = (char*)(cldp + 1);
            printf("Received HELLO from %s: %.*s\n", 
                   inet_ntoa(*(struct in_addr*)&ip->saddr), 
                   cldp->payload_len, payload);
        }

        // Send HELLO every 10 seconds (skeleton)
        static time_t last_hello = 0;
        time_t now = time(NULL);
        if (now - last_hello >= 10) {
            char hello_buf[512];
            struct iphdr *hello_ip = (struct iphdr*)hello_buf;
            struct cldp_header *hello_cldp = (struct cldp_header*)(hello_buf + sizeof(struct iphdr));

            // Fill IP header (skeleton)
            hello_ip->version = 4;
            hello_ip->ihl = 5;
            hello_ip->protocol = CLDP_PROTOCOL;
            hello_ip->saddr = inet_addr("192.168.1.100"); // Your IP
            hello_ip->daddr = inet_addr("255.255.255.255"); // Broadcast

            // Fill CLDP header for HELLO
            hello_cldp->type = 0x01;
            char *payload = (char*)(hello_cldp + 1);
            strcpy(payload, "Hi");
            hello_cldp->payload_len = strlen("Hi");

            // Set total length and checksum (skeleton)
            hello_ip->tot_len = htons(sizeof(struct iphdr) + sizeof(struct cldp_header) + hello_cldp->payload_len);
            hello_ip->check = ip_checksum(hello_ip, sizeof(struct iphdr));

            // Send HELLO
            struct sockaddr_in broadcast_addr = {.sin_family = AF_INET, .sin_addr.s_addr = INADDR_BROADCAST};
            sendto(sock, hello_buf, ntohs(hello_ip->tot_len), 0, 
                   (struct sockaddr*)&broadcast_addr, sizeof(broadcast_addr));
            last_hello = now;
        }
    }

    close(sock);
    return 0;
}