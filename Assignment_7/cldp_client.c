#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <time.h>
#include <ifaddrs.h>
#include <sys/time.h>
#include <errno.h>

#define PROTOCOL_NUM 253
#define BUFFER_SIZE 1024

// Custom Lightweight Discovery Protocol header: 8 bytes
struct cldp_header {
    uint8_t msg_type;      // Message Type (1 byte)
    uint8_t payload_len;   // Payload Length (1 byte)
    uint16_t trans_id;     // Transaction ID (2 bytes)
    uint32_t reserved;     // Reserved (4 bytes)
};

// Calculate checksum for the IP header
uint16_t ip_checksum(void *buf, int len) {
    uint32_t sum = 0;
    uint16_t *ptr = (uint16_t *)buf;
    while (len > 1) {
        sum += *ptr++;
        len -= 2;
    }
    if (len)
        sum += *(uint8_t *)ptr;
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    return (uint16_t)(~sum);
}

// Debug function to print packet details
void debug_packet(const char *prefix, struct iphdr *iph, struct cldp_header *cldph, const char *payload) {
    char src_ip[INET_ADDRSTRLEN], dst_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &iph->saddr, src_ip, INET_ADDRSTRLEN);
    inet_ntop(AF_INET, &iph->daddr, dst_ip, INET_ADDRSTRLEN);
    printf("[DEBUG] %s: IP[%s -> %s, proto=%d, len=%d] CLDP[type=%d, trans_id=%u] Payload: %s\n",
           prefix, src_ip, dst_ip, iph->protocol, ntohs(iph->tot_len),
           cldph->msg_type, ntohs(cldph->trans_id), payload);
}

// Get the non-loopback local IP address
char *get_local_ip() {
    struct ifaddrs *ifaddr, *ifa;
    static char ip[INET_ADDRSTRLEN] = "0.0.0.0";
    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs failed");
        return ip;
    }
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET) {
            struct sockaddr_in *sa = (struct sockaddr_in *)ifa->ifa_addr;
            inet_ntop(AF_INET, &(sa->sin_addr), ip, INET_ADDRSTRLEN);
            if (strcmp(ifa->ifa_name, "lo") != 0)
                break;
        }
    }
    freeifaddrs(ifaddr);
    return ip;
}

int main() {
    int sock = socket(AF_INET, SOCK_RAW, PROTOCOL_NUM);
    if (sock < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
    
    int opt = 1;
    if (setsockopt(sock, IPPROTO_IP, IP_HDRINCL, &opt, sizeof(opt)) < 0) {
        perror("setsockopt IP_HDRINCL failed");
        exit(EXIT_FAILURE);
    }
    if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &opt, sizeof(opt)) < 0) {
        perror("setsockopt SO_BROADCAST failed");
        exit(EXIT_FAILURE);
    }
    
    char buffer[BUFFER_SIZE];
    char *local_ip = get_local_ip();
    printf("Client IP: %s\n", local_ip);
    
    // Prepare a broadcast QUERY.
    // For this demo, we send a query requesting the hostname (query type 0x01).
    // The client will then listen for responses from all active servers.
    struct sockaddr_in dest_addr = {
        .sin_family = AF_INET,
        .sin_port = 0,
        .sin_addr.s_addr = inet_addr("255.255.255.255")
    };
    
    struct iphdr iph = {0};
    struct cldp_header cldp = {0x02, 1, rand() % 65535, 0};  // QUERY (msg_type 0x02)
    uint8_t query_type = 0x01;  // 0x01 for hostname query; change to 0x02 for timestamp query if desired
    
    iph.version = 4;
    iph.ihl = 5;
    iph.ttl = 64;
    iph.tos = 0;
    iph.id = htons(rand() % 65535);
    iph.frag_off = 0;
    iph.tot_len = htons(sizeof(iph) + sizeof(cldp) + sizeof(query_type));
    iph.protocol = PROTOCOL_NUM;
    iph.saddr = inet_addr(local_ip);
    iph.daddr = dest_addr.sin_addr.s_addr;
    iph.check = 0;
    iph.check = ip_checksum(&iph, sizeof(iph));
    
    memcpy(buffer, &iph, sizeof(iph));
    memcpy(buffer + sizeof(iph), &cldp, sizeof(cldp));
    memcpy(buffer + sizeof(iph) + sizeof(cldp), &query_type, sizeof(query_type));
    
    debug_packet("QUERY_OUT", &iph, &cldp, (query_type == 0x01 ? "Hostname Query" : "Timestamp Query"));
    
    int sent = sendto(sock, buffer, ntohs(iph.tot_len), 0,
                      (struct sockaddr *)&dest_addr, sizeof(dest_addr));
    if (sent < 0) {
        perror("Failed to send QUERY");
        exit(EXIT_FAILURE);
    }
    printf("Sent QUERY for %s (Trans ID: %u)\n", (query_type == 0x01 ? "hostname" : "timestamp"), cldp.trans_id);
    
    // Collect responses from all active servers for a specified time window (e.g., 3 seconds)
    printf("Waiting for responses...\n");
    time_t timeout = time(NULL) + 3; // 3-second window
    while(time(NULL) < timeout) {
        FD_ZERO(&((fd_set){0}));
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(sock, &readfds);
        
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 500000; // wait for 500 ms
        int ret = select(sock + 1, &readfds, NULL, NULL, &tv);
        if (ret <= 0)
            continue;
        int len = recvfrom(sock, buffer, BUFFER_SIZE, 0, NULL, NULL);
        if (len > 0) {
            struct iphdr *iph_rx = (struct iphdr *)buffer;
            if (iph_rx->protocol == PROTOCOL_NUM) {
                struct cldp_header *cldp_rx = (struct cldp_header *)(buffer + sizeof(*iph_rx));
                int payload_offset = sizeof(*iph_rx) + sizeof(*cldp_rx);
                int pay_len = ntohs(iph_rx->tot_len) - payload_offset;
                char received_payload[BUFFER_SIZE];
                if (pay_len > 0 && pay_len < BUFFER_SIZE) {
                    memcpy(received_payload, buffer + payload_offset, pay_len);
                    received_payload[pay_len - 1] = '\0';
                } else {
                    received_payload[0] = '\0';
                }
                debug_packet("RESPONSE_IN", iph_rx, cldp_rx, received_payload);
                printf("Received RESPONSE (Trans ID: %u): %s\n", ntohs(cldp_rx->trans_id), received_payload);
            }
        }
    }
    
    close(sock);
    return 0;
}