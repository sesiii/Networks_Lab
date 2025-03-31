// Assignment 7 Submission
// Name: <Your_Name>
// Roll number: <Your_Roll_Number>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <ifaddrs.h>
#include <sys/time.h>

#define PROTOCOL_NUM 253
#define BUFFER_SIZE 1024

struct cldp_header {
    uint8_t msg_type;
    uint8_t payload_len;
    uint16_t trans_id;
    uint32_t reserved;
};

uint16_t ip_checksum(void *buf, int len) {
    uint32_t sum = 0;
    uint16_t *ptr = (uint16_t *)buf;
    while (len > 1) {
        sum += *ptr++;
        len -= 2;
    }
    if (len) sum += *(uint8_t *)ptr;
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    return (uint16_t)(~sum);
}

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
            if (strcmp(ifa->ifa_name, "lo") != 0) break;
        }
    }
    freeifaddrs(ifaddr);
    return ip;
}

int main() {
    int sock = socket(AF_INET, SOCK_RAW, PROTOCOL_NUM);
    if (sock < 0) {
        perror("Socket creation failed");
        exit(1);
    }

    int opt = 1;
    if (setsockopt(sock, IPPROTO_IP, IP_HDRINCL, &opt, sizeof(opt)) < 0 ||
        setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &opt, sizeof(opt)) < 0) {
        perror("setsockopt failed");
        close(sock);
        exit(1);
    }

    struct sockaddr_in dest_addr = { .sin_family = AF_INET, .sin_addr.s_addr = inet_addr("255.255.255.255") };
    char buffer[BUFFER_SIZE];
    char *local_ip = get_local_ip();
    printf("Client IP: %s\n", local_ip);

    while (1) {
        fd_set readfds;
        struct timeval tv = { .tv_sec = 1, .tv_usec = 0 };
        FD_ZERO(&readfds);
        FD_SET(sock, &readfds);

        if (select(sock + 1, &readfds, NULL, NULL, &tv) > 0) {
            int len = recvfrom(sock, buffer, BUFFER_SIZE, 0, NULL, NULL);
            if (len > 0) {
                struct iphdr *iph = (struct iphdr *)buffer;
                if (iph->protocol == PROTOCOL_NUM) {
                    struct cldp_header *cldp = (struct cldp_header *)(buffer + sizeof(*iph));
                    if (cldp->msg_type == 0x01) {
                        printf("Received HELLO from %s (Trans ID: %u)\n",
                               inet_ntoa(*(struct in_addr *)&iph->saddr), ntohs(cldp->trans_id));
                    }
                }
            }
        }

        printf("\nEnter metadata type (1: Hostname, 2: Time, 3: CPU Load, 0: Exit): ");
        int query_type;
        scanf("%d", &query_type);
        if (query_type == 0) break;
        if (query_type < 1 || query_type > 3) {
            printf("Invalid query type.\n");
            continue;
        }

        struct iphdr iph = { .version = 4, .ihl = 5, .ttl = 64, .protocol = PROTOCOL_NUM };
        uint16_t trans_id = rand() % 65535;
        struct cldp_header cldp = { 0x02, 1, htons(trans_id), 0 };
        uint8_t payload = (uint8_t)query_type;
        iph.id = htons(rand() % 65535);
        iph.tot_len = htons(sizeof(iph) + sizeof(cldp) + 1);
        iph.saddr = inet_addr(local_ip);
        iph.daddr = dest_addr.sin_addr.s_addr;
        iph.check = ip_checksum(&iph, sizeof(iph));

        memcpy(buffer, &iph, sizeof(iph));
        memcpy(buffer + sizeof(iph), &cldp, sizeof(cldp));
        memcpy(buffer + sizeof(iph) + sizeof(cldp), &payload, 1);

        if (sendto(sock, buffer, ntohs(iph.tot_len), 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0) {
            perror("Send QUERY failed");
            continue;
        }
        printf("Sent QUERY for type %d (Trans ID: %u)\n", query_type, trans_id);

        while (1) {
            FD_ZERO(&readfds);
            FD_SET(sock, &readfds);
            tv.tv_sec = 2; // Timeout for response
            if (select(sock + 1, &readfds, NULL, NULL, &tv) <= 0) {
                printf("No response received.\n");
                break;
            }

            int len = recvfrom(sock, buffer, BUFFER_SIZE, 0, NULL, NULL);
            if (len > 0) {
                struct iphdr *rx_iph = (struct iphdr *)buffer;
                if (rx_iph->protocol == PROTOCOL_NUM) {
                    struct cldp_header *rx_cldp = (struct cldp_header *)(buffer + sizeof(*rx_iph));
                    if (rx_cldp->msg_type == 0x03 && ntohs(rx_cldp->trans_id) == trans_id) {
                        char *payload = buffer + sizeof(*rx_iph) + sizeof(*rx_cldp);
                        payload[rx_cldp->payload_len - 1] = '\0';
                        printf("Response from %s: %s (Trans ID: %u)\n",
                               inet_ntoa(*(struct in_addr *)&rx_iph->saddr), payload, trans_id);
                    }
                }
            }
        }
    }

    close(sock);
    return 0;
}