// // Assignment 7 Submission
// // Name: <Your_Name>
// // Roll number: <Your_Roll_Number>

// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <unistd.h>
// #include <arpa/inet.h>
// #include <sys/socket.h>
// #include <netinet/ip.h>
// #include <time.h>
// #include <ifaddrs.h>
// #include <sys/time.h>

// #define PROTOCOL_NUM 253
// #define BUFFER_SIZE 1024
// #define HELLO_INTERVAL 10

// struct cldp_header {
//     uint8_t msg_type;
//     uint8_t payload_len;
//     uint16_t trans_id;
//     uint32_t reserved;
// };

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

// int verify_ip_checksum(struct iphdr *iph) {
//     uint16_t original = iph->check;
//     iph->check = 0;
//     uint16_t computed = ip_checksum(iph, iph->ihl * 4);
//     iph->check = original;
//     return computed == original;
// }

// char *get_local_ip() {
//     struct ifaddrs *ifaddr, *ifa;
//     static char ip[INET_ADDRSTRLEN] = "0.0.0.0";
//     if (getifaddrs(&ifaddr) == -1) {
//         perror("getifaddrs failed");
//         return ip;
//     }
//     for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
//         if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET) {
//             struct sockaddr_in *sa = (struct sockaddr_in *)ifa->ifa_addr;
//             inet_ntop(AF_INET, &(sa->sin_addr), ip, INET_ADDRSTRLEN);
//             if (strcmp(ifa->ifa_name, "lo") != 0) break;
//         }
//     }
//     freeifaddrs(ifaddr);
//     return ip;
// }

// int main() {
//     int sock = socket(AF_INET, SOCK_RAW, PROTOCOL_NUM);
//     if (sock < 0) {
//         perror("Socket creation failed");
//         close(sock);
//         exit(1);
//     }

//     int opt = 1;
//     if (setsockopt(sock, IPPROTO_IP, IP_HDRINCL, &opt, sizeof(opt)) < 0 ||
//         setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &opt, sizeof(opt)) < 0) {
//         perror("setsockopt failed");
//         close(sock);
//         exit(1);
//     }

//     struct sockaddr_in dest_addr = {
//         .sin_family = AF_INET,
//         .sin_addr.s_addr = inet_addr("255.255.255.255")
//     };

//     char buffer[BUFFER_SIZE];
//     time_t last_hello = 0;
//     char *local_ip = get_local_ip();
//     printf("Server IP: %s\n", local_ip);
//     printf("Server started. Listening for CLDP packets on protocol %d...\n", PROTOCOL_NUM);

//     while (1) {
//         time_t now = time(NULL);
//         if (now - last_hello >= HELLO_INTERVAL) {
//             struct iphdr iph = { .version = 4, .ihl = 5, .ttl = 64, .protocol = PROTOCOL_NUM };
//             uint16_t trans_id = rand() % 65535;
//             struct cldp_header cldp = { 0x01, 0, htons(trans_id), 0 };
//             char hello_payload[] = "HELLO";
//             iph.id = htons(rand() % 65535);
//             iph.tot_len = htons(sizeof(iph) + sizeof(cldp) + sizeof(hello_payload));
//             iph.saddr = inet_addr(local_ip);
//             iph.daddr = dest_addr.sin_addr.s_addr;
//             iph.check = ip_checksum(&iph, sizeof(iph));

//             memcpy(buffer, &iph, sizeof(iph));
//             memcpy(buffer + sizeof(iph), &cldp, sizeof(cldp));
//             memcpy(buffer + sizeof(iph) + sizeof(cldp), hello_payload, sizeof(hello_payload));

//             if (sendto(sock, buffer, ntohs(iph.tot_len), 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0) {
//                 perror("Failed to send HELLO broadcast");
//             } else {
//                 printf("Sent HELLO (Trans ID: %u)\n", trans_id);
//             }
//             last_hello = now;
//         }

//         fd_set readfds;
//         struct timeval tv = { .tv_sec = 0, .tv_usec = 100000 };
//         FD_ZERO(&readfds);
//         FD_SET(sock, &readfds);

//         if (select(sock + 1, &readfds, NULL, NULL, &tv) <= 0) continue;

//         int len = recvfrom(sock, buffer, BUFFER_SIZE, 0, NULL, NULL);
//         if (len <= 0) continue;

//         struct iphdr *iph_rx = (struct iphdr *)buffer;
//         if (iph_rx->protocol != PROTOCOL_NUM || !verify_ip_checksum(iph_rx)) continue;

//         struct cldp_header *cldp_rx = (struct cldp_header *)(buffer + sizeof(*iph_rx));
//         if (cldp_rx->msg_type != 0x02) continue;

//         int payload_len = ntohs(iph_rx->tot_len) - sizeof(*iph_rx) - sizeof(*cldp_rx);
//         if (payload_len <= 0) continue;

//         uint8_t *payload_rx = (uint8_t *)(buffer + sizeof(*iph_rx) + sizeof(*cldp_rx));
//         char response_data[256] = "N/A";
//         if (payload_rx[0] == 0x01) gethostname(response_data, sizeof(response_data));
//         else if (payload_rx[0] == 0x02) {
//             struct timeval tv;
//             gettimeofday(&tv, NULL);
//             snprintf(response_data, sizeof(response_data), "%ld.%06ld", tv.tv_sec, tv.tv_usec);
//         } else if (payload_rx[0] == 0x03) {
//             FILE *fp = popen("uptime | awk '{print $10}'", "r");
//             if (fp) {
//                 fgets(response_data, sizeof(response_data), fp);
//                 pclose(fp);
//                 response_data[strcspn(response_data, "\n")] = 0;
//             }
//         }

//         struct iphdr iph_tx = { .version = 4, .ihl = 5, .ttl = 64, .protocol = PROTOCOL_NUM };
//         struct cldp_header cldp_tx = { 0x03, strlen(response_data) + 1, cldp_rx->trans_id, 0 };
//         iph_tx.id = htons(rand() % 65535);
//         iph_tx.tot_len = htons(sizeof(iph_tx) + sizeof(cldp_tx) + cldp_tx.payload_len);
//         iph_tx.saddr = inet_addr(local_ip);
//         iph_tx.daddr = iph_rx->saddr;
//         iph_tx.check = ip_checksum(&iph_tx, sizeof(iph_tx));

//         char tx_buffer[BUFFER_SIZE];
//         memcpy(tx_buffer, &iph_tx, sizeof(iph_tx));
//         memcpy(tx_buffer + sizeof(iph_tx), &cldp_tx, sizeof(cldp_tx));
//         memcpy(tx_buffer + sizeof(iph_tx) + sizeof(cldp_tx), response_data, cldp_tx.payload_len);

//         struct sockaddr_in reply_addr = { .sin_family = AF_INET, .sin_addr.s_addr = iph_rx->saddr };
//         if (sendto(sock, tx_buffer, ntohs(iph_tx.tot_len), 0, (struct sockaddr *)&reply_addr, sizeof(reply_addr)) < 0) {
//             perror("Failed to send RESPONSE");
//         } else {
//             printf("Sent RESPONSE to %s (Type: %d, Data: %s, Trans ID: %u)\n",
//                    inet_ntoa(*(struct in_addr *)&iph_rx->saddr), payload_rx[0], response_data, ntohs(cldp_rx->trans_id));
//         }
//     }

//     close(sock);
//     return 0;
// }

//2
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
#include <time.h>
#include <ifaddrs.h>
#include <sys/time.h>

#define PROTOCOL_NUM 253
#define BUFFER_SIZE 1024
#define HELLO_INTERVAL 10

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

int verify_ip_checksum(struct iphdr *iph) {
    uint16_t original = iph->check;
    iph->check = 0;
    uint16_t computed = ip_checksum(iph, iph->ihl * 4);
    iph->check = original;
    return computed == original;
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
    time_t last_hello = 0;
    char *local_ip = get_local_ip();
    uint32_t local_ip_int = inet_addr(local_ip);
    printf("Server IP: %s\n", local_ip);
    printf("Server started. Listening for CLDP packets on protocol %d...\n", PROTOCOL_NUM);

    static uint16_t server_trans_id = 10000; // Start server IDs at 10000 to avoid client overlap

    while (1) {
        time_t now = time(NULL);
        if (now - last_hello >= HELLO_INTERVAL) {
            struct iphdr iph = { .version = 4, .ihl = 5, .ttl = 64, .protocol = PROTOCOL_NUM };
            uint16_t trans_id = server_trans_id++;
            struct cldp_header cldp = { 0x01, 0, htons(trans_id), 0 };
            char hello_payload[] = "HELLO";
            iph.id = htons(rand() % 65535);
            iph.tot_len = htons(sizeof(iph) + sizeof(cldp) + sizeof(hello_payload));
            iph.saddr = local_ip_int;
            iph.daddr = dest_addr.sin_addr.s_addr;
            iph.check = ip_checksum(&iph, sizeof(iph));

            memcpy(buffer, &iph, sizeof(iph));
            memcpy(buffer + sizeof(iph), &cldp, sizeof(cldp));
            memcpy(buffer + sizeof(iph) + sizeof(cldp), hello_payload, sizeof(hello_payload));

            if (sendto(sock, buffer, ntohs(iph.tot_len), 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0) {
                perror("Failed to send HELLO broadcast");
            } else {
                printf("Sent HELLO (Trans ID: %u)\n", trans_id);
            }
            last_hello = now;
        }

        fd_set readfds;
        struct timeval tv = { .tv_sec = 0, .tv_usec = 100000 };
        FD_ZERO(&readfds);
        FD_SET(sock, &readfds);

        if (select(sock + 1, &readfds, NULL, NULL, &tv) <= 0) continue;

        int len = recvfrom(sock, buffer, BUFFER_SIZE, 0, NULL, NULL);
        if (len <= 0) continue;

        struct iphdr *iph_rx = (struct iphdr *)buffer;
        if (iph_rx->protocol != PROTOCOL_NUM || !verify_ip_checksum(iph_rx)) {
            printf("Received invalid packet (proto=%d, len=%d)\n", iph_rx->protocol, len);
            continue;
        }

        struct cldp_header *cldp_rx = (struct cldp_header *)(buffer + sizeof(*iph_rx));
        if (cldp_rx->msg_type == 0x02) {
            int payload_len = ntohs(iph_rx->tot_len) - sizeof(*iph_rx) - sizeof(*cldp_rx);
            if (payload_len <= 0) continue;

            uint8_t *payload_rx = (uint8_t *)(buffer + sizeof(*iph_rx) + sizeof(*cldp_rx));
            printf("Received QUERY from %s (Type: %d, Trans ID: %u)\n",
                   inet_ntoa(*(struct in_addr *)&iph_rx->saddr), payload_rx[0], ntohs(cldp_rx->trans_id));

            char response_data[256] = "N/A";
            if (payload_rx[0] == 0x01) {
                gethostname(response_data, sizeof(response_data));
            } else if (payload_rx[0] == 0x02) {
                struct timeval tv;
                gettimeofday(&tv, NULL);
                snprintf(response_data, sizeof(response_data), "%ld.%06ld", tv.tv_sec, tv.tv_usec);
            } else if (payload_rx[0] == 0x03) {
                FILE *fp = popen("uptime | awk '{print $10}' | tr -d ','", "r");
                if (fp) {
                    fgets(response_data, sizeof(response_data), fp);
                    pclose(fp);
                    response_data[strcspn(response_data, "\n")] = 0;
                }
            }

            struct iphdr iph_tx = { .version = 4, .ihl = 5, .ttl = 64, .protocol = PROTOCOL_NUM };
            struct cldp_header cldp_tx = { 0x03, strlen(response_data) + 1, cldp_rx->trans_id, 0 };
            iph_tx.id = htons(rand() % 65535);
            iph_tx.tot_len = htons(sizeof(iph_tx) + sizeof(cldp_tx) + cldp_tx.payload_len);
            iph_tx.saddr = local_ip_int;
            iph_tx.daddr = iph_rx->saddr;
            iph_tx.check = ip_checksum(&iph_tx, sizeof(iph_tx));

            char tx_buffer[BUFFER_SIZE];
            memcpy(tx_buffer, &iph_tx, sizeof(iph_tx));
            memcpy(tx_buffer + sizeof(iph_tx), &cldp_tx, sizeof(cldp_tx));
            memcpy(tx_buffer + sizeof(iph_tx) + sizeof(cldp_tx), response_data, cldp_tx.payload_len);

            struct sockaddr_in reply_addr = { .sin_family = AF_INET, .sin_addr.s_addr = iph_rx->saddr };
            if (sendto(sock, tx_buffer, ntohs(iph_tx.tot_len), 0, (struct sockaddr *)&reply_addr, sizeof(reply_addr)) < 0) {
                perror("Failed to send RESPONSE");
            } else {
                printf("Sent RESPONSE to %s (Type: %d, Data: %s, Trans ID: %u)\n",
                       inet_ntoa(*(struct in_addr *)&iph_rx->saddr), payload_rx[0], response_data, ntohs(cldp_rx->trans_id));
            }
        }
    }

    close(sock);
    return 0;
}