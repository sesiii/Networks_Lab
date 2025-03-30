// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <unistd.h>
// #include <arpa/inet.h>
// #include <sys/socket.h>
// #include <netinet/ip.h>
// #include <ifaddrs.h>
// #include <netdb.h>

// #define PROTOCOL_NUM 253
// #define BUFFER_SIZE 1024

// // CLDP Header Structure
// struct cldp_header {
//     uint8_t msg_type;      // Message Type (1 byte)
//     uint8_t payload_len;   // Payload Length (1 byte)
//     uint16_t trans_id;     // Transaction ID (2 bytes)
//     uint32_t reserved;     // Reserved (4 bytes)
// };

// // IP Checksum function
// uint16_t ip_checksum(void *buf, int len) {
//     uint32_t sum = 0;
//     uint16_t *ptr = (uint16_t *)buf;
//     while (len > 1) {
//         sum += *ptr++;
//         len -= 2;
//     }
//     if (len) sum += *(uint8_t *)ptr;
//     sum = (sum >> 16) + (sum & 0xFFFF);
//     sum += (sum >> 16);
//     return (uint16_t)(~sum);
// }

// // Function to get local IP dynamically
// char *get_local_ip() {
//     struct ifaddrs *ifaddr, *ifa;
//     static char ip[INET_ADDRSTRLEN];

//     if (getifaddrs(&ifaddr) == -1) {
//         perror("getifaddrs failed");
//         return "0.0.0.0";
//     }

//     for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
//         if (ifa->ifa_addr == NULL || ifa->ifa_addr->sa_family != AF_INET) continue;
//         if (strcmp(ifa->ifa_name, "lo") == 0) continue; // Skip loopback
//         struct sockaddr_in *sa = (struct sockaddr_in *)ifa->ifa_addr;
//         inet_ntop(AF_INET, &(sa->sin_addr), ip, INET_ADDRSTRLEN);
//         break;
//     }

//     freeifaddrs(ifaddr);
//     return ip[0] ? ip : "0.0.0.0";
// }

// int main() {
//     int sock = socket(AF_INET, SOCK_RAW, PROTOCOL_NUM);
//     if (sock < 0) {
//         perror("Socket creation failed");
//         exit(1);
//     }

//     int opt = 1;
//     if (setsockopt(sock, IPPROTO_IP, IP_HDRINCL, &opt, sizeof(opt)) < 0) {
//         perror("setsockopt IP_HDRINCL failed");
//         exit(1);
//     }
//     if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &opt, sizeof(opt)) < 0) {
//         perror("setsockopt SO_BROADCAST failed");
//         exit(1);
//     }

//     struct sockaddr_in dest_addr;
//     dest_addr.sin_family = AF_INET;
//     dest_addr.sin_addr.s_addr = inet_addr("255.255.255.255");

//     char buffer[BUFFER_SIZE];
//     char *local_ip = get_local_ip();
//     printf("Client IP: %s\n", local_ip);

//     // Send QUERY for each metadata type
//     uint8_t metadata_types[] = {0x01, 0x02, 0x03}; // Hostname, Time, CPU Load
//     for (int i = 0; i < 3; i++) {
//         struct iphdr iph = {0};
//         struct cldp_header cldp = {0x02, 1, rand() % 65535, 0}; // QUERY
//         uint8_t payload = metadata_types[i];

//         iph.version = 4;
//         iph.ihl = 5;
//         iph.tot_len = htons(sizeof(iph) + sizeof(cldp) + 1);
//         iph.protocol = PROTOCOL_NUM;
//         iph.saddr = inet_addr(local_ip);
//         iph.daddr = dest_addr.sin_addr.s_addr;
//         iph.check = ip_checksum(&iph, sizeof(iph));

//         memcpy(buffer, &iph, sizeof(iph));
//         memcpy(buffer + sizeof(iph), &cldp, sizeof(cldp));
//         memcpy(buffer + sizeof(iph) + sizeof(cldp), &payload, 1);

//         if (sendto(sock, buffer, ntohs(iph.tot_len), 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0) {
//             perror("Send QUERY failed");
//         } else {
//             printf("Sent QUERY for metadata type %d (Trans ID: %u)\n", payload, cldp.trans_id);
//         }

//         // Receive RESPONSE
//         int len = recvfrom(sock, buffer, BUFFER_SIZE, 0, NULL, NULL);
//         if (len > 0) {
//             struct iphdr *rx_iph = (struct iphdr *)buffer;
//             if (rx_iph->protocol == PROTOCOL_NUM) {
//                 struct cldp_header *rx_cldp = (struct cldp_header *)(buffer + sizeof(*rx_iph));
//                 if (rx_cldp->msg_type == 0x03) {
//                     printf("Received RESPONSE (Trans ID: %u): ", rx_cldp->trans_id);
//                     char *payload = buffer + sizeof(*rx_iph) + sizeof(*rx_cldp) + 1;
//                     printf("%s\n", payload);
//                 }
//             }
//         }
//         sleep(1); // Wait between queries
//     }

//     close(sock);
//     return 0;
// }



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <ifaddrs.h>
#include <netdb.h>

#define PROTOCOL_NUM 253
#define BUFFER_SIZE 1024

// CLDP Header Structure
struct cldp_header {
    uint8_t msg_type;      // Message Type (1 byte)
    uint8_t payload_len;   // Payload Length (1 byte)
    uint16_t trans_id;     // Transaction ID (2 bytes)
    uint32_t reserved;     // Reserved (4 bytes)
};

// IP Checksum function
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

// Function to get local IP dynamically
char *get_local_ip() {
    struct ifaddrs *ifaddr, *ifa;
    static char ip[INET_ADDRSTRLEN];

    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs failed");
        return "0.0.0.0";
    }

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL || ifa->ifa_addr->sa_family != AF_INET) continue;
        if (strcmp(ifa->ifa_name, "lo") == 0) continue; // Skip loopback
        struct sockaddr_in *sa = (struct sockaddr_in *)ifa->ifa_addr;
        inet_ntop(AF_INET, &(sa->sin_addr), ip, INET_ADDRSTRLEN);
        break;
    }

    freeifaddrs(ifaddr);
    return ip[0] ? ip : "0.0.0.0";
}

int main() {
    int sock = socket(AF_INET, SOCK_RAW, PROTOCOL_NUM);
    if (sock < 0) {
        perror("Socket creation failed");
        exit(1);
    }

    int opt = 1;
    if (setsockopt(sock, IPPROTO_IP, IP_HDRINCL, &opt, sizeof(opt)) < 0) {
        perror("setsockopt IP_HDRINCL failed");
        exit(1);
    }
    if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &opt, sizeof(opt)) < 0) {
        perror("setsockopt SO_BROADCAST failed");
        exit(1);
    }

    struct sockaddr_in dest_addr;
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_addr.s_addr = inet_addr("255.255.255.255");

    char buffer[BUFFER_SIZE];
    char *local_ip = get_local_ip();
    printf("Client IP: %s\n", local_ip);

    // Loop to send one query at a time and wait for a response
    uint8_t metadata_types[] = {0x01, 0x02, 0x03}; // Hostname, Time, CPU Load
    for (int i = 0; i < 3; i++) {
        struct iphdr iph = {0};
        struct cldp_header cldp = {0x02, 1, rand() % 65535, 0}; // QUERY
        uint8_t payload = metadata_types[i];

        iph.version = 4;
        iph.ihl = 5;
        iph.tot_len = htons(sizeof(iph) + sizeof(cldp) + 1);
        iph.protocol = PROTOCOL_NUM;
        iph.saddr = inet_addr(local_ip);
        iph.daddr = dest_addr.sin_addr.s_addr;
        iph.check = ip_checksum(&iph, sizeof(iph));

        memcpy(buffer, &iph, sizeof(iph));
        memcpy(buffer + sizeof(iph), &cldp, sizeof(cldp));
        memcpy(buffer + sizeof(iph) + sizeof(cldp), &payload, 1);

        if (sendto(sock, buffer, ntohs(iph.tot_len), 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0) {
            perror("Send QUERY failed");
        } else {
            printf("Sent QUERY for metadata type %d (Trans ID: %u)\n", payload, cldp.trans_id);
        }

        // Receive RESPONSE
        int len = recvfrom(sock, buffer, BUFFER_SIZE, 0, NULL, NULL);
        if (len > 0) {
            struct iphdr *rx_iph = (struct iphdr *)buffer;
            if (rx_iph->protocol == PROTOCOL_NUM) {
                struct cldp_header *rx_cldp = (struct cldp_header *)(buffer + sizeof(*rx_iph));
                if (rx_cldp->msg_type == 0x03) {
                    printf("Received RESPONSE (Trans ID: %u): ", rx_cldp->trans_id);
                    char *payload = buffer + sizeof(*rx_iph) + sizeof(*rx_cldp) + 1;
                    printf("%s\n", payload);
                }
            }
        } else {
            perror("Receive RESPONSE failed");
        }
        sleep(1); // Wait between queries
    }

    close(sock);
    return 0;
}