/*
 ============================================================================
 Assignment 7 Submission
 Name: <Your Name>
 Roll number: <Your Roll Number>
 Description : CLDP Client using Raw Sockets
 ============================================================================
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>     // Required for sockaddr_in, IPPROTO_IP
#include <netinet/ip.h>     // Required for struct iphdr
#include <ifaddrs.h>
#include <netdb.h>
#include <time.h>
#include <sys/time.h>       // Required for gettimeofday, timeval
#include <errno.h>          // Required for errno
#include <poll.h>           // Required for poll() for timeout

#define PROTOCOL_NUM 253
#define BUFFER_SIZE 1024
#define RESPONSE_TIMEOUT_MS 2000 // Timeout for waiting for response in milliseconds

// CLDP Header Structure
struct cldp_header {
    uint8_t msg_type;      // Message Type (1 byte)
    uint8_t payload_len;   // Payload Length (1 byte) - Size of CLDP payload ONLY
    uint16_t trans_id;     // Transaction ID (2 bytes) - Network Byte Order
    uint32_t reserved;     // Reserved (4 bytes)
};

// IP Checksum function (same as server)
uint16_t ip_checksum(void *vdata, size_t length) {
    char *data = (char *)vdata;
    uint32_t sum = 0;
    uint16_t *ptr = (uint16_t *)data;
    size_t i;

    for (i = length; i > 1; i -= 2) {
        sum += *ptr++;
    }
    if (i == 1) {
        sum += *(uint8_t *)ptr;
    }
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    return (uint16_t)(~sum);
}

// Function to get local IP dynamically (same as server)
char *get_local_ip() {
    struct ifaddrs *ifaddr, *ifa;
    static char ip[INET_ADDRSTRLEN] = "0.0.0.0";
    int family;

    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs failed");
        return ip;
    }
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL) continue;
        family = ifa->ifa_addr->sa_family;
        if (family == AF_INET && strcmp(ifa->ifa_name, "lo") != 0) {
            struct sockaddr_in *sa = (struct sockaddr_in *)ifa->ifa_addr;
             if (inet_ntop(AF_INET, &(sa->sin_addr), ip, INET_ADDRSTRLEN) != NULL) {
                 break; // Found one
             } else {
                 perror("inet_ntop failed");
                 // Keep default if conversion fails
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
    char *local_ip = get_local_ip();

    // Seed random number generator
    srand(time(NULL) ^ getpid());

    printf("Assignment 7: CLDP Client\n");
    printf("Client IP: %s\n", local_ip);
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

    // 3. Set SO_BROADCAST socket option (needed for sending QUERY)
    if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &opt, sizeof(opt)) < 0) {
        perror("setsockopt SO_BROADCAST failed");
        close(sock);
        exit(EXIT_FAILURE);
    }

    // Prepare broadcast destination address
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = 0; // Port is irrelevant
     if (inet_pton(AF_INET, "255.255.255.255", &dest_addr.sin_addr) <= 0) {
         perror("inet_pton for broadcast failed");
         close(sock);
         exit(EXIT_FAILURE);
    }


    while (1) {
        printf("\nAvailable Metadata Query Types:\n");
        printf("  1: Hostname\n");
        printf("  2: System Time (Timestamp)\n");
        printf("  3: CPU Load (1-min avg)\n");
        printf("Enter metadata type number to query (or 0 to Exit): ");

        int query_type_input;
        if (scanf("%d", &query_type_input) != 1) {
             printf("Invalid input. Please enter a number.\n");
             // Clear input buffer
             while (getchar() != '\n');
             continue;
        }
        // Clear trailing newline
        while (getchar() != '\n');

        if (query_type_input == 0) {
            printf("Exiting...\n");
            break;
        }

        if (query_type_input < 1 || query_type_input > 3) {
            printf("Invalid query type '%d'. Please enter 1, 2, 3, or 0.\n", query_type_input);
            continue;
        }

        uint8_t query_payload_byte = (uint8_t)query_type_input;

        // --- Prepare and Send QUERY ---
        struct iphdr *iph_query = (struct iphdr *)buffer;
        struct cldp_header *cldph_query = (struct cldp_header *)(buffer + sizeof(struct iphdr));

        // Zero out buffer portion
        memset(buffer, 0, sizeof(struct iphdr) + sizeof(struct cldp_header) + sizeof(query_payload_byte));

        // Store host-order transaction ID for later comparison
        uint16_t host_trans_id = rand() % 65535;

        // Fill CLDP Header
        cldph_query->msg_type = 0x02; // QUERY
        cldph_query->payload_len = sizeof(query_payload_byte); // Payload is 1 byte
        cldph_query->trans_id = htons(host_trans_id); // Network byte order
        cldph_query->reserved = 0;

        // Copy payload
        memcpy((char *)cldph_query + sizeof(struct cldp_header), &query_payload_byte, sizeof(query_payload_byte));

        // Fill IP Header
        iph_query->version = 4;
        iph_query->ihl = 5; // 20 bytes
        iph_query->tos = 0;
        iph_query->tot_len = htons(sizeof(struct iphdr) + sizeof(struct cldp_header) + cldph_query->payload_len);
        iph_query->id = htons(rand() % 65535);
        iph_query->frag_off = 0;
        iph_query->ttl = 64;
        iph_query->protocol = PROTOCOL_NUM;
        iph_query->check = 0; // To calculate checksum
        if (inet_pton(AF_INET, local_ip, &iph_query->saddr) <= 0) {
            perror("inet_pton for local_ip failed"); continue;
        }
        iph_query->daddr = dest_addr.sin_addr.s_addr; // Broadcast

        // Calculate IP Checksum
        iph_query->check = ip_checksum(iph_query, sizeof(struct iphdr));

        // Send the QUERY packet
        if (sendto(sock, buffer, ntohs(iph_query->tot_len), 0,
                   (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0) {
            perror("Send QUERY failed");
            continue; // Try again
        } else {
            printf("Sent QUERY for metadata type %d (Trans ID: %u)\n", query_payload_byte, host_trans_id);
        }

        // --- Wait for RESPONSE with Timeout ---
        printf("Waiting for RESPONSE(s)...\n");
        int responses_found = 0;

        // Use poll for timeout handling
        struct pollfd pfd;
        pfd.fd = sock;
        pfd.events = POLLIN;

        // Loop to potentially catch multiple responses within the timeout window
        while(1) {
            int poll_ret = poll(&pfd, 1, RESPONSE_TIMEOUT_MS); // Wait for timeout

            if (poll_ret < 0) {
                perror("poll failed");
                break; // Exit receive loop on error
            } else if (poll_ret == 0) {
                // Timeout occurred
                if (!responses_found) {
                     printf("Timeout: No matching RESPONSE received within %d ms.\n", RESPONSE_TIMEOUT_MS);
                }
                break; // Exit receive loop
            } else {
                 // Data available (pfd.revents & POLLIN should be true)
                 char recv_buffer[BUFFER_SIZE];
                 struct sockaddr_in resp_addr_in;
                 socklen_t resp_addr_len = sizeof(resp_addr_in);

                 ssize_t len = recvfrom(sock, recv_buffer, BUFFER_SIZE, 0,
                                        (struct sockaddr *)&resp_addr_in, &resp_addr_len);

                 if (len < 0) {
                     if (errno == EINTR) continue; // Interrupted, try again
                     perror("recvfrom failed");
                     break; // Exit receive loop on error
                 }

                 if (len < sizeof(struct iphdr)) continue; 

                 struct iphdr *iph_rx = (struct iphdr *)recv_buffer;

                 // Filter by protocol
                 if (iph_rx->protocol != PROTOCOL_NUM) continue;

                 unsigned int ip_hdr_len = iph_rx->ihl * 4;
                 if (len < ip_hdr_len + sizeof(struct cldp_header)) continue; 

                 struct cldp_header *cldp_rx = (struct cldp_header *)(recv_buffer + ip_hdr_len);

                 // Checks if it's a RESPONSE and matches the Transaction ID
                 if ((cldp_rx->msg_type == 0x03  && ntohs(cldp_rx->trans_id) == host_trans_id)) {
                     char resp_ip_str[INET_ADDRSTRLEN];
                     inet_ntop(AF_INET, &(iph_rx->saddr), resp_ip_str, INET_ADDRSTRLEN);

                     size_t expected_total_len = ip_hdr_len + sizeof(struct cldp_header) + cldp_rx->payload_len;
                     if (len < expected_total_len || cldp_rx->payload_len == 0) {
                         fprintf(stderr, "Received RESPONSE from %s with invalid/incomplete payload (expected %zu, got %zd, clpdp_len %d)\n",
                                 resp_ip_str, expected_total_len, len, cldp_rx->payload_len);
                         continue;
                     }


                     char *payload_data = recv_buffer + ip_hdr_len + sizeof(struct cldp_header);
                     if (cldp_rx->payload_len > 0) {
                         payload_data[cldp_rx->payload_len - 1] = '\0';
                     } else {
                         payload_data[0] = '\0'; // Handle zero-length payload case
                     }


                     printf("Received RESPONSE from %s (Trans ID: %u): %s\n",
                            resp_ip_str, ntohs(cldp_rx->trans_id), payload_data);
                     responses_found++;
                 }
            }
        } 
    } 

    printf("Client shutting down.\n");
    close(sock);
    return 0;
}