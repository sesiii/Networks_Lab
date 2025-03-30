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

// Custom Lightweight Discovery Protocol header: 8 bytes
struct cldp_header {
    uint8_t msg_type;      // Message Type (1 byte)
    uint8_t payload_len;   // Payload Length (1 byte)
    uint16_t trans_id;     // Transaction ID (2 bytes)
    uint32_t reserved;     // Reserved (4 bytes)
};

// Calculate checksum for a buffer (used for IP header)
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

// Verify that the IP header checksum is correct.
// It temporarily resets the checksum field to zero, calculates the checksum and compares it.
int verify_ip_checksum(struct iphdr *iph) {
    uint16_t original = iph->check;
    iph->check = 0;
    uint16_t computed = ip_checksum(iph, iph->ihl * 4);
    iph->check = original;  // Restore the checksum field
    if(computed == original) {
        printf("Checksum verification PASSED. Checksum: 0x%x\n", original);
        return 1;
    } else {
        printf("Checksum verification FAILED. Original: 0x%x, Computed: 0x%x\n", original, computed);
        return 0;
    }
}

// Debug function to print packet details, including the payload (if printable)
void debug_packet(const char *prefix, struct iphdr *iph, struct cldp_header *cldph, const char *payload) {
    char src_ip[INET_ADDRSTRLEN], dst_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &iph->saddr, src_ip, INET_ADDRSTRLEN);
    inet_ntop(AF_INET, &iph->daddr, dst_ip, INET_ADDRSTRLEN);
    printf("[DEBUG] %s: IP[%s->%s, proto=%d, len=%d] CLDP[type=%d, trans_id=%u] Payload: %s\n",
           prefix, src_ip, dst_ip, iph->protocol, ntohs(iph->tot_len),
           cldph->msg_type, ntohs(cldph->trans_id), payload);
}

// Get the IP address of a non-loopback interface
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
    if (setsockopt(sock, IPPROTO_IP, IP_HDRINCL, &opt, sizeof(opt)) < 0) {
        perror("setsockopt IP_HDRINCL failed");
        exit(1);
    }
    if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &opt, sizeof(opt)) < 0) {
        perror("setsockopt SO_BROADCAST failed");
        exit(1);
    }

    struct sockaddr_in dest_addr = {
        .sin_family = AF_INET,
        .sin_port = 0,  // Port is not used for raw sockets
        .sin_addr.s_addr = inet_addr("255.255.255.255")
    };

    char buffer[BUFFER_SIZE];
    time_t last_hello = 0;
    char *local_ip = get_local_ip();
    printf("Server IP: %s\n", local_ip);
    printf("Server started. Listening for CLDP packets on protocol %d...\n", PROTOCOL_NUM);

    while (1) {
        time_t now = time(NULL);
        // Send HELLO broadcast every HELLO_INTERVAL seconds using the required identifier string
        if (now - last_hello >= HELLO_INTERVAL) {
            struct iphdr iph = {0};
            uint16_t trans_id = htons(rand() % 65535);  // network byte order
            struct cldp_header cldp = {0x01, 0, trans_id, 0};
            // Use the identifier as payload instead of just "HELLO"
            char hello_payload[] = "HELLO";

            iph.version = 4;
            iph.ihl = 5;
            iph.ttl = 64;
            iph.tos = 0;
            iph.id = htons(rand() % 65535);
            iph.frag_off = 0;
            iph.tot_len = htons(sizeof(iph) + sizeof(cldp) + sizeof(hello_payload));
            iph.protocol = PROTOCOL_NUM;
            iph.saddr = inet_addr(local_ip);
            iph.daddr = dest_addr.sin_addr.s_addr;
            iph.check = 0;
            iph.check = ip_checksum(&iph, sizeof(iph));
            // Verify the checksum before sending
            verify_ip_checksum(&iph);

            memcpy(buffer, &iph, sizeof(iph));
            memcpy(buffer + sizeof(iph), &cldp, sizeof(cldp));
            memcpy(buffer + sizeof(iph) + sizeof(cldp), hello_payload, sizeof(hello_payload));

            debug_packet("HELLO_OUT", &iph, &cldp, hello_payload);

            int broadcast_result = sendto(sock, buffer, ntohs(iph.tot_len), 0,
                                          (struct sockaddr *)&dest_addr, sizeof(dest_addr));
            if (broadcast_result < 0) {
                perror("Failed to send HELLO broadcast");
            } else {
                printf("Sent HELLO (Trans ID: %u) - %d bytes\n", ntohs(trans_id), broadcast_result);
            }
            last_hello = now;
        }

        // Use select() to avoid indefinite blocking on recvfrom()
        fd_set readfds;
        struct timeval tv;
        FD_ZERO(&readfds);
        FD_SET(sock, &readfds);
        tv.tv_sec = 0;
        tv.tv_usec = 100000;  // 100ms

        if (select(sock + 1, &readfds, NULL, NULL, &tv) <= 0)
            continue;  // No data available

        // Process incoming packets
        int len = recvfrom(sock, buffer, BUFFER_SIZE, 0, NULL, NULL);
        if (len > 0) {
            struct iphdr *iph_rx = (struct iphdr *)buffer;
            if (iph_rx->protocol == PROTOCOL_NUM) {
                struct cldp_header *cldp_rx = (struct cldp_header *)(buffer + sizeof(*iph_rx));
                // Extract payload from received packet
                int payload_offset = sizeof(*iph_rx) + sizeof(*cldp_rx);
                int pay_len = ntohs(iph_rx->tot_len) - payload_offset;
                char received_payload[BUFFER_SIZE];
                if (pay_len > 0 && pay_len < BUFFER_SIZE) {
                    memcpy(received_payload, buffer + payload_offset, pay_len);
                    received_payload[pay_len - 1] = '\0';
                } else {
                    received_payload[0] = '\0';
                }
                debug_packet("PACKET_IN", iph_rx, cldp_rx, received_payload);

                // Process QUERY messages (msg_type 0x02)
                if (cldp_rx->msg_type == 0x02) {
                    char src_ip[INET_ADDRSTRLEN];
                    inet_ntop(AF_INET, &iph_rx->saddr, src_ip, INET_ADDRSTRLEN);
                    uint8_t *payload_rx = (uint8_t *)(buffer + sizeof(*iph_rx) + sizeof(*cldp_rx));
                    printf("Received QUERY from %s (Type: %d, Trans ID: %u)\n",
                           src_ip, payload_rx[0], ntohs(cldp_rx->trans_id));

                    // Prepare response packet with msg_type 0x03
                    struct iphdr iph_tx = {0};
                    struct cldp_header cldp_tx = {0x03, 0, cldp_rx->trans_id, 0}; // use same transaction ID
                    char tx_buffer[BUFFER_SIZE];
                    char response_data[256] = "N/A";

                    if (payload_rx[0] == 0x01) {
                        // QUERY for hostname
                        gethostname(response_data, sizeof(response_data));
                    } else if (payload_rx[0] == 0x02) {
                        // QUERY for system time
                        struct timeval tv;
                        gettimeofday(&tv, NULL);
                        snprintf(response_data, sizeof(response_data), "%ld.%06ld", tv.tv_sec, tv.tv_usec);
                    } else if (payload_rx[0] == 0x03) {
                        // QUERY for CPU load (simplified)
                        FILE *fp = popen("uptime | awk '{print $10}'", "r");
                        if (fp) {
                            fgets(response_data, sizeof(response_data), fp);
                            pclose(fp);
                            response_data[strcspn(response_data, "\n")] = 0;
                        }
                    }

                    cldp_tx.payload_len = strlen(response_data) + 1;
                    iph_tx.version = 4;
                    iph_tx.ihl = 5;
                    iph_tx.ttl = 64;
                    iph_tx.tos = 0;
                    iph_tx.id = htons(rand() % 65535);
                    iph_tx.frag_off = 0;
                    iph_tx.tot_len = htons(sizeof(iph_tx) + sizeof(cldp_tx) + cldp_tx.payload_len);
                    iph_tx.protocol = PROTOCOL_NUM;
                    iph_tx.saddr = inet_addr(local_ip);
                    iph_tx.daddr = iph_rx->saddr;
                    iph_tx.check = 0;
                    iph_tx.check = ip_checksum(&iph_tx, sizeof(iph_tx));
                    // Verify checksum for outgoing response
                    verify_ip_checksum(&iph_tx);

                    memcpy(tx_buffer, &iph_tx, sizeof(iph_tx));
                    memcpy(tx_buffer + sizeof(iph_tx), &cldp_tx, sizeof(cldp_tx));
                    memcpy(tx_buffer + sizeof(iph_tx) + sizeof(cldp_tx), response_data, cldp_tx.payload_len);

                    debug_packet("RESPONSE_OUT", &iph_tx, &cldp_tx, response_data);

                    struct sockaddr_in reply_addr = {
                        .sin_family = AF_INET,
                        .sin_port = 0,
                        .sin_addr.s_addr = iph_rx->saddr
                    };

                    int send_result = sendto(sock, tx_buffer, ntohs(iph_tx.tot_len), 0,
                                               (struct sockaddr *)&reply_addr, sizeof(reply_addr));
                    if (send_result < 0)
                        perror("Failed to send RESPONSE");
                    else
                        printf("Sent RESPONSE (Type: %d, Data: %s, Trans ID: %u)\n",
                               payload_rx[0], response_data, ntohs(cldp_rx->trans_id));
                }
            }
        }
    }
    close(sock);
    return 0;
}