#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <time.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <sys/time.h>

#define PROTOCOL_NUM 253
#define BUFFER_SIZE 1024

// CLDP Header Structure
struct cldp_header {
    uint8_t msg_type;      // Message Type (1 byte)
    uint8_t payload_len;   // Payload Length (1 byte)
    uint16_t trans_id;     // Transaction ID (2 bytes)
    uint32_t reserved;     // Reserved (4 bytes)
};

// Function to calculate IP checksum
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
    // Create raw socket
    int sock = socket(AF_INET, SOCK_RAW, PROTOCOL_NUM);
    if (sock < 0) {
        perror("Socket creation failed");
        exit(1);
    }

    // Enable IP_HDRINCL and SO_BROADCAST
    int opt = 1;
    if (setsockopt(sock, IPPROTO_IP, IP_HDRINCL, &opt, sizeof(opt)) < 0) {
        perror("setsockopt IP_HDRINCL failed");
        exit(1);
    }
    if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &opt, sizeof(opt)) < 0) {
        perror("setsockopt SO_BROADCAST failed");
        exit(1);
    }

    // Destination address for HELLO (broadcast)
    struct sockaddr_in dest_addr;
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_addr.s_addr = inet_addr("255.255.255.255");

    char buffer[BUFFER_SIZE];
    time_t last_hello = 0;
    char *local_ip = get_local_ip();
    printf("Server IP: %s\n", local_ip);

    while (1) {
        // Send HELLO every 10 seconds
        time_t now = time(NULL);
        if (now - last_hello >= 10) {
            struct iphdr iph = {0};
            struct cldp_header cldp = {0x01, 0, rand() % 65535, 0}; // HELLO message
            iph.version = 4;
            iph.ihl = 5;
            iph.tot_len = htons(sizeof(iph) + sizeof(cldp));
            iph.protocol = PROTOCOL_NUM;
            iph.saddr = inet_addr(local_ip);
            iph.daddr = dest_addr.sin_addr.s_addr;
            iph.check = ip_checksum(&iph, sizeof(iph));

            memcpy(buffer, &iph, sizeof(iph));
            memcpy(buffer + sizeof(iph), &cldp, sizeof(cldp));
            if (sendto(sock, buffer, ntohs(iph.tot_len), 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0) {
                perror("Send HELLO failed");
            } else {
                printf("Sent HELLO (Trans ID: %u)\n", cldp.trans_id);
            }
            last_hello = now;
        }

        // Receive and process incoming packets
        int len = recvfrom(sock, buffer, BUFFER_SIZE, 0, NULL, NULL);
        if (len > 0) {
            struct iphdr *iph_rx = (struct iphdr *)buffer;
            if (iph_rx->protocol == PROTOCOL_NUM) {
                struct cldp_header *cldp_rx = (struct cldp_header *)(buffer + sizeof(*iph_rx));
                if (cldp_rx->msg_type == 0x02) { // QUERY
                    uint8_t *payload_rx = (uint8_t *)(buffer + sizeof(*iph_rx) + sizeof(*cldp_rx));
                    struct iphdr iph_tx = {0};
                    struct cldp_header cldp_tx = {0x03, 0, cldp_rx->trans_id, 0};
                    char tx_buffer[BUFFER_SIZE];
                    char *response_data = tx_buffer + sizeof(iph_tx) + sizeof(cldp_tx) + 1;

                    if (payload_rx[0] == 0x01) { // Hostname
                        char hostname[256];
                        gethostname(hostname, sizeof(hostname));
                        cldp_tx.payload_len = strlen(hostname) + 1;
                        memcpy(response_data, hostname, strlen(hostname));
                    } else if (payload_rx[0] == 0x02) { // System time
                        struct timeval tv;
                        gettimeofday(&tv, NULL);
                        char time_str[32];
                        snprintf(time_str, sizeof(time_str), "%ld.%06ld", tv.tv_sec, tv.tv_usec);
                        cldp_tx.payload_len = strlen(time_str) + 1;
                        memcpy(response_data, time_str, strlen(time_str));
                    } else if (payload_rx[0] == 0x03) { // CPU load (simplified)
                        FILE *fp = popen("uptime | awk '{print $10}'", "r");
                        char load[16] = "N/A";
                        if (fp) {
                            fgets(load, sizeof(load), fp);
                            pclose(fp);
                            load[strcspn(load, "\n")] = 0; // Remove newline
                        }
                        cldp_tx.payload_len = strlen(load) + 1;
                        memcpy(response_data, load, strlen(load));
                    } else {
                        continue; // Unknown metadata type
                    }

                    iph_tx.version = 4;
                    iph_tx.ihl = 5;
                    iph_tx.tot_len = htons(sizeof(iph_tx) + sizeof(cldp_tx) + cldp_tx.payload_len);
                    iph_tx.protocol = PROTOCOL_NUM;
                    iph_tx.saddr = inet_addr(local_ip);
                    iph_tx.daddr = iph_rx->saddr;
                    iph_tx.check = ip_checksum(&iph_tx, sizeof(iph_tx));

                    memcpy(tx_buffer, &iph_tx, sizeof(iph_tx));
                    memcpy(tx_buffer + sizeof(iph_tx), &cldp_tx, sizeof(cldp_tx));
                    tx_buffer[sizeof(iph_tx) + sizeof(cldp_tx)] = payload_rx[0]; // Metadata type

                    struct sockaddr_in reply_addr;
                    reply_addr.sin_family = AF_INET;
                    reply_addr.sin_addr.s_addr = iph_rx->saddr;
                    if (sendto(sock, tx_buffer, ntohs(iph_tx.tot_len), 0, (struct sockaddr *)&reply_addr, sizeof(reply_addr)) < 0) {
                        perror("Send RESPONSE failed");
                    } else {
                        printf("Sent RESPONSE (Type: %d, Data: %s)\n", payload_rx[0], response_data);
                    }
                }
            }
        }

        usleep(100000); // Sleep 100ms
    }

    close(sock);
    return 0;
}