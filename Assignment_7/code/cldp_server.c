/*
 ============================================================================
 Assignment 7 Submission
 Name: <Your Name>
 Roll number: <Your Roll Number>
 Description : CLDP Server using Raw Sockets
 ============================================================================
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h> // Required for sockaddr_in, IPPROTO_IP
#include <netinet/ip.h> // Required for struct iphdr
#include <time.h>
#include <ifaddrs.h>
#include <sys/time.h>
#include <sys/select.h> // Required for select()
#include <errno.h>      // Required for errno

#define PROTOCOL_NUM 253
#define BUFFER_SIZE 1024
#define HELLO_INTERVAL 10 // Seconds

// Custom Lightweight Discovery Protocol header: 8 bytes
struct cldp_header {
    uint8_t msg_type;      // Message Type (1 byte)
    uint8_t payload_len;   // Payload Length (1 byte) - Size of CLDP payload ONLY
    uint16_t trans_id;     // Transaction ID (2 bytes) - Network Byte Order
    uint32_t reserved;     // Reserved (4 bytes)
};

// Calculate checksum for a buffer (used for IP header)
// Standard algorithm
uint16_t ip_checksum(void *vdata, size_t length) {
    char *data = (char *)vdata;
    uint32_t sum = 0;
    uint16_t *ptr = (uint16_t *)data;
    size_t i;

    // Main loop - process 16-bit chunks
    for (i = length; i > 1; i -= 2) {
        sum += *ptr++;
    }

    // Add left-over byte, if any
    if (i == 1) {
        sum += *(uint8_t *)ptr;
    }

    // Fold 32-bit sum to 16 bits
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    return (uint16_t)(~sum);
}


// Get the IP address of a non-loopback interface
char *get_local_ip() {
    struct ifaddrs *ifaddr, *ifa;
    static char ip[INET_ADDRSTRLEN] = "0.0.0.0"; // Default
    int family;

    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs failed");
        return ip; // Return default
    }

    // Iterate through interfaces
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL) continue;

        family = ifa->ifa_addr->sa_family;

        // Check if it is IPv4 and not loopback
        if (family == AF_INET && strcmp(ifa->ifa_name, "lo") != 0) {
            struct sockaddr_in *sa = (struct sockaddr_in *)ifa->ifa_addr;
            if (inet_ntop(AF_INET, &(sa->sin_addr), ip, INET_ADDRSTRLEN) == NULL) {
                 perror("inet_ntop failed");
                 // Keep default IP if conversion fails
            } else {
                // Found a suitable IP, break loop
                break;
            }
        }
    }

    freeifaddrs(ifaddr);
    return ip;
}

int main() {
    int sock;
    struct sockaddr_in dest_addr;
    char buffer[BUFFER_SIZE];
    time_t last_hello_time = 0;
    char *local_ip = get_local_ip();
    struct timeval tv;
    struct tm *tm_info;
    char response_data[64];

    // Seed random number generator (for transaction IDs)
    srand(time(NULL) ^ getpid());

    printf("Assignment 7: CLDP Server\n");
    printf("Server IP: %s\n", local_ip);
    if (strcmp(local_ip, "0.0.0.0") == 0) {
        fprintf(stderr, "Error: Could not determine a valid local IP address. Exiting.\n");
        exit(EXIT_FAILURE);
    }


    // 1. Create Raw Socket
    sock = socket(AF_INET, SOCK_RAW, PROTOCOL_NUM);
    if (sock < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // 2. Set IP_HDRINCL socket option
    int opt = 1;
    if (setsockopt(sock, IPPROTO_IP, IP_HDRINCL, &opt, sizeof(opt)) < 0) {
        perror("setsockopt IP_HDRINCL failed");
        close(sock);
        exit(EXIT_FAILURE);
    }

    // 3. Set SO_BROADCAST socket option (needed for sending HELLO)
    if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &opt, sizeof(opt)) < 0) {
        perror("setsockopt SO_BROADCAST failed");
        close(sock);
        exit(EXIT_FAILURE);
    }

    // Prepare broadcast destination address
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = 0; // Port is irrelevant for raw IP
    if (inet_pton(AF_INET, "255.255.255.255", &dest_addr.sin_addr) <= 0) {
         perror("inet_pton for broadcast failed");
         close(sock);
         exit(EXIT_FAILURE);
    }


    printf("Server started. Listening for CLDP packets (protocol %d)...\n", PROTOCOL_NUM);

    while (1) {
        time_t current_time = time(NULL);

        // --- Send Periodic HELLO ---
        if (current_time - last_hello_time >= HELLO_INTERVAL) {
            struct iphdr *iph_hello = (struct iphdr *)buffer;
            struct cldp_header *cldph_hello = (struct cldp_header *)(buffer + sizeof(struct iphdr));
            char hello_payload[] = "HELLO"; // Example payload for HELLO

            // Zero out buffer portion
            memset(buffer, 0, sizeof(struct iphdr) + sizeof(struct cldp_header) + sizeof(hello_payload));

            // Fill CLDP Header
            cldph_hello->msg_type = 0x01; // HELLO
            cldph_hello->payload_len = sizeof(hello_payload); // Including null terminator
            cldph_hello->trans_id = htons(rand() % 65535); // Network byte order
            cldph_hello->reserved = 0;

            // Copy payload
            memcpy((char *)cldph_hello + sizeof(struct cldp_header), hello_payload, sizeof(hello_payload));

            // Fill IP Header
            iph_hello->version = 4;
            iph_hello->ihl = 5; // 5 * 4 = 20 bytes (no options)
            iph_hello->tos = 0;
            iph_hello->tot_len = htons(sizeof(struct iphdr) + sizeof(struct cldp_header) + cldph_hello->payload_len);
            iph_hello->id = htons(rand() % 65535); // Identification
            iph_hello->frag_off = 0; // No fragmentation
            iph_hello->ttl = 64;     // Time To Live
            iph_hello->protocol = PROTOCOL_NUM;
            iph_hello->check = 0; // Checksum needs to be 0 before calculation
            if (inet_pton(AF_INET, local_ip, &iph_hello->saddr) <= 0) {
                perror("inet_pton for local_ip failed"); continue; // Skip sending hello this time
            }
            iph_hello->daddr = dest_addr.sin_addr.s_addr; // Broadcast address

            // Calculate IP Checksum
            iph_hello->check = ip_checksum(iph_hello, sizeof(struct iphdr));

            // Send the packet
            if (sendto(sock, buffer, ntohs(iph_hello->tot_len), 0,
                       (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0) {
                perror("Send HELLO failed");
            } else {
                printf("Sent HELLO (Trans ID: %u, Payload: '%s')\n", ntohs(cldph_hello->trans_id), hello_payload);
            }
            last_hello_time = current_time;
        }

        fd_set readfds;
        struct timeval tv;
        FD_ZERO(&readfds);
        FD_SET(sock, &readfds);

        tv.tv_sec = 0;
        tv.tv_usec = 100000; 

        int activity = select(sock + 1, &readfds, NULL, NULL, &tv);

        if (activity < 0 && errno != EINTR) {
            perror("select error");
        } else if (activity > 0 && FD_ISSET(sock, &readfds)) {
            // Data is available to read
            struct sockaddr_in src_addr_in;
            socklen_t src_addr_len = sizeof(src_addr_in);
            char recv_buffer[BUFFER_SIZE];
            ssize_t len = recvfrom(sock, recv_buffer, BUFFER_SIZE, 0,
                                   (struct sockaddr *)&src_addr_in, &src_addr_len);

            if (len < 0) {
                perror("recvfrom failed");
                continue;
            }

            if (len < sizeof(struct iphdr)) {
                 fprintf(stderr, "Received packet too small for IP header\n");
                 continue;
            }

            struct iphdr *iph_rx = (struct iphdr *)recv_buffer;

            // protocol no validation
            if (iph_rx->protocol != PROTOCOL_NUM) {
                continue;
            }

            // Check if packet is long enough for CLDP header
            unsigned int ip_hdr_len = iph_rx->ihl * 4;
            if (len < ip_hdr_len + sizeof(struct cldp_header)) {
                fprintf(stderr, "Received packet too small for CLDP header\n");
                continue;
            }

            //checksum validation
            uint16_t original_checksum = iph_rx->check;
            iph_rx->check = 0;
            if (ip_checksum(iph_rx, ip_hdr_len) != original_checksum) {
                fprintf(stderr, "Received packet with invalid IP checksum\n");
                iph_rx->check = original_checksum; // Restore for potential further processing
                continue;
            }
            iph_rx->check = original_checksum; 

            struct cldp_header *cldp_rx = (struct cldp_header *)(recv_buffer + ip_hdr_len);
            char src_ip_str[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &(iph_rx->saddr), src_ip_str, INET_ADDRSTRLEN);


            // Process only QUERY messages
            if (cldp_rx->msg_type == 0x02) { // QUERY
                // Check payload length reported by CLDP header
                if (cldp_rx->payload_len < 1) {
                    fprintf(stderr, "Received QUERY from %s with invalid payload length (%d)\n", src_ip_str, cldp_rx->payload_len);
                    continue;
                }
                // Check if actual received data length matches expected length
                size_t expected_total_len = ip_hdr_len + sizeof(struct cldp_header) + cldp_rx->payload_len;
                 if (len < expected_total_len) {
                    fprintf(stderr, "Received QUERY from %s with less data (%zd) than expected (%zu)\n", src_ip_str, len, expected_total_len);
                    continue; // Incomplete packet
                 }


                uint8_t *payload_rx = (uint8_t *)(recv_buffer + ip_hdr_len + sizeof(struct cldp_header));
                uint8_t query_type = payload_rx[0]; // First byte indicates query type

                printf("Received QUERY from %s (Type: %d, Trans ID: %u)\n",
                       src_ip_str, query_type, ntohs(cldp_rx->trans_id));

                // --- Prepare and Send RESPONSE ---
                char tx_buffer[BUFFER_SIZE];
                char response_data[BUFFER_SIZE - sizeof(struct iphdr) - sizeof(struct cldp_header) - 10]; // Reserve space
                memset(response_data, 0, sizeof(response_data));
                int data_ok = 1;

                switch (query_type) {
                    case 1: // Hostname
                        if (gethostname(response_data, sizeof(response_data) - 1) != 0) {
                            perror("gethostname failed");
                            strncpy(response_data, "Error: Hostname", sizeof(response_data)-1);
                            data_ok = 0;
                        }
                        break;
                    // case 2: // System Time (Unix timestamp with microseconds)
                    //     {
                    //         struct timeval tv;
                    //         // struct tm *tm_info;
                    //         // char response_data[64];
                    //         gettimeofday(&tv, NULL);
                    //         // tm_info = localtime(&tv.tv_sec);
                    //         // strftime(response_data, sizeof(response_data), "%Y-%m-%d %H:%M:%S", tm_info);
                    //         // snprintf(response_data + strlen(response_data), sizeof(response_data) - strlen(response_data), ".%06ld", (long)tv.tv_usec);
                    //         snprintf(response_data, sizeof(response_data), "%ld.%06ld", tv.tv_sec, (long)tv.tv_usec);
                    //     }
                    case 2: // System Time (Formatted Date-Time with Microseconds)
{
    struct timeval tv;
    struct tm *tm_info;
    gettimeofday(&tv, NULL);
    tm_info = localtime(&tv.tv_sec);

    strftime(response_data, sizeof(response_data), "%Y-%m-%d %H:%M:%S", tm_info);
    snprintf(response_data + strlen(response_data), sizeof(response_data) - strlen(response_data), ".%06ld", (long)tv.tv_usec);
}
                        break;
                    case 3: // CPU Load (1-min average from uptime)
                        {
                            FILE *fp = popen("uptime | awk -F'load average: ' '{print $2}' | awk -F, '{print $1}'", "r");
                            if (fp) {
                                if (fgets(response_data, sizeof(response_data) -1 , fp) == NULL) {
                                    if (ferror(fp)) perror("fgets from uptime failed");
                                    strncpy(response_data, "Error: Read Load", sizeof(response_data)-1);
                                    data_ok = 0;
                                }
                                pclose(fp);
                                // Remove trailing newline if present
                                response_data[strcspn(response_data, "\n")] = 0;
                            } else {
                                perror("popen for uptime failed");
                                strncpy(response_data, "Error: Get Load", sizeof(response_data)-1);
                                data_ok = 0;
                            }
                        }
                        break;
                    default:
                        snprintf(response_data, sizeof(response_data), "Error: Unknown Query Type (%d)", query_type);
                        data_ok = 0;
                        break;
                }

                 // If data couldn't be retrieved, send an error response payload
                if (!data_ok) {
                    printf("Failed to get data for query type %d\n", query_type);
                    // response_data already contains the error message
                }


                struct iphdr *iph_tx = (struct iphdr *)tx_buffer;
                struct cldp_header *cldp_tx = (struct cldp_header *)(tx_buffer + sizeof(struct iphdr));

                // Zero out header portion
                memset(tx_buffer, 0, sizeof(struct iphdr) + sizeof(struct cldp_header));

                // Fill CLDP Header
                cldp_tx->msg_type = 0x03; // RESPONSE
                cldp_tx->payload_len = strlen(response_data) + 1; // Include null terminator
                cldp_tx->trans_id = cldp_rx->trans_id; // Use same Trans ID (already in network order)
                cldp_tx->reserved = 0;

                // Copy payload
                memcpy((char *)cldp_tx + sizeof(struct cldp_header), response_data, cldp_tx->payload_len);

                // Fill IP Header
                iph_tx->version = 4;
                iph_tx->ihl = 5;
                iph_tx->tos = 0;
                iph_tx->tot_len = htons(sizeof(struct iphdr) + sizeof(struct cldp_header) + cldp_tx->payload_len);
                iph_tx->id = htons(rand() % 65535);
                iph_tx->frag_off = 0;
                iph_tx->ttl = 64;
                iph_tx->protocol = PROTOCOL_NUM;
                iph_tx->check = 0; 
                 if (inet_pton(AF_INET, local_ip, &iph_tx->saddr) <= 0) {
                    perror("inet_pton for local_ip failed"); continue; 
                 }
                iph_tx->daddr = iph_rx->saddr; 

                iph_tx->check = ip_checksum(iph_tx, sizeof(struct iphdr));

                struct sockaddr_in reply_addr;
                memset(&reply_addr, 0, sizeof(reply_addr));
                reply_addr.sin_family = AF_INET;
                reply_addr.sin_addr.s_addr = iph_tx->daddr; 

                // Send the response
                if (sendto(sock, tx_buffer, ntohs(iph_tx->tot_len), 0,
                           (struct sockaddr *)&reply_addr, sizeof(reply_addr)) < 0) {
                    perror("Send RESPONSE failed");
                } else {
                    printf("Sent RESPONSE to %s (Type: %d, Data: '%s', Trans ID: %u)\n",
                           src_ip_str, query_type, response_data, ntohs(cldp_tx->trans_id));
                }
            }   
        }
    }

    printf("Server shutting down.\n");
    close(sock);
    return 0;
}