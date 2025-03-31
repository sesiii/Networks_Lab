#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <ifaddrs.h>
#include <fcntl.h>
#include <sys/select.h>
#include <time.h>
#include <errno.h>
#include <signal.h>

#define PROTOCOL_NUM 253
#define BUFFER_SIZE 1024
#define MAX_SERVERS 32
#define QUERY_TIMEOUT 2

// Signal flag for clean termination
volatile int running = 1;

struct cldp_header {
    uint8_t msg_type;
    uint8_t payload_len;
    uint16_t trans_id;
    uint32_t reserved;
};

struct server_info {
    uint32_t ip_addr;
    time_t last_seen;
    char hostname[64];
    int active;
};

// Global variables
struct server_info servers[MAX_SERVERS];
uint32_t local_ip_int;
static uint16_t client_trans_id = 0;
int debug_mode = 1;

// Function prototypes
uint16_t ip_checksum(void *buf, int len);
char *get_local_ip();
void update_server_list(uint32_t ip_addr);
void print_servers();
void send_query(int sock, int query_type);
void handle_response(struct iphdr *iph, struct cldp_header *cldp, char *payload);
void handle_hello(struct iphdr *iph, struct cldp_header *cldp);
void signal_handler(int sig);

void signal_handler(int sig) {
    printf("\nReceived signal %d, exiting...\n", sig);
    running = 0;
}

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

void update_server_list(uint32_t ip_addr) {
    int i;
    // Skip our own IP
    // if (ip_addr == local_ip_int) return;
    if (ip_addr == local_ip_int && !debug_mode) return;
    
    // Check if server already exists
    for (i = 0; i < MAX_SERVERS; i++) {
        if (servers[i].active && servers[i].ip_addr == ip_addr) {
            servers[i].last_seen = time(NULL);
            return;
        }
    }
    
    // Add new server
    for (i = 0; i < MAX_SERVERS; i++) {
        if (!servers[i].active) {
            servers[i].ip_addr = ip_addr;
            servers[i].last_seen = time(NULL);
            servers[i].active = 1;
            memset(servers[i].hostname, 0, sizeof(servers[i].hostname));
            if (debug_mode) {
                printf("New server discovered: %s\n", inet_ntoa(*(struct in_addr *)&ip_addr));
            }
            return;
        }
    }
    
    // If we get here, our server table is full
    if (debug_mode) {
        printf("Warning: Server table full, cannot add more servers\n");
    }
}

void print_servers() {
    time_t now = time(NULL);
    int i, active_count = 0;
    
    printf("\n--- Active Servers ---\n");
    for (i = 0; i < MAX_SERVERS; i++) {
        if (servers[i].active) {
            // Consider servers inactive after 30 seconds without a HELLO
            if (now - servers[i].last_seen > 30) {
                servers[i].active = 0;
                continue;
            }
            
            printf("%d. %s - Last seen: %ld seconds ago\n", 
                   ++active_count,
                   inet_ntoa(*(struct in_addr *)&servers[i].ip_addr),
                   now - servers[i].last_seen);
            
            if (servers[i].hostname[0] != '\0') {
                printf("   Hostname: %s\n", servers[i].hostname);
            }
        }
    }
    
    if (active_count == 0) {
        printf("No active servers found\n");
    }
    printf("---------------------\n");
}

void send_query(int sock, int query_type) {
    char buffer[BUFFER_SIZE];
    struct sockaddr_in dest_addr = { .sin_family = AF_INET, .sin_addr.s_addr = inet_addr("255.255.255.255") };
    uint16_t trans_id = client_trans_id++; 
    
    struct iphdr iph = { 
        .version = 4, 
        .ihl = 5, 
        .ttl = 64, 
        .protocol = PROTOCOL_NUM,
        .id = htons(rand() % 65535)
    };
    
    struct cldp_header cldp = { 
        .msg_type = 0x02, 
        .payload_len = 1, 
        .trans_id = htons(trans_id), 
        .reserved = 0 
    };
    
    uint8_t payload = (uint8_t)query_type;
    
    iph.tot_len = htons(sizeof(iph) + sizeof(cldp) + 1);
    iph.saddr = local_ip_int;
    iph.daddr = dest_addr.sin_addr.s_addr;
    iph.check = ip_checksum(&iph, sizeof(iph));

    memcpy(buffer, &iph, sizeof(iph));
    memcpy(buffer + sizeof(iph), &cldp, sizeof(cldp));
    memcpy(buffer + sizeof(iph) + sizeof(cldp), &payload, 1);

    if (sendto(sock, buffer, ntohs(iph.tot_len), 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            perror("Send QUERY failed");
        }
        return;
    }
    
    printf("Sent QUERY for type %d (Trans ID: %u)\n", query_type, trans_id);
    return;
}

void handle_response(struct iphdr *iph, struct cldp_header *cldp, char *payload) {
    printf("Response from %s: %.*s (Trans ID: %u)\n",
        inet_ntoa(*(struct in_addr *)&iph->saddr), 
        cldp->payload_len, payload, 
        ntohs(cldp->trans_id));
           
    // Update server info for the responding server
    update_server_list(iph->saddr);
}

void handle_hello(struct iphdr *iph, struct cldp_header *cldp) {
    update_server_list(iph->saddr);
    
    if (debug_mode) {
        printf("Received HELLO from %s (Trans ID: %u)\n",
               inet_ntoa(*(struct in_addr *)&iph->saddr), 
               ntohs(cldp->trans_id));
    }
}

int main(int argc, char *argv[]) {
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--debug") == 0) {
            debug_mode = 1;
        }
    }

    // Set up signal handlers for graceful termination
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // Create socket
    int sock = socket(AF_INET, SOCK_RAW, PROTOCOL_NUM);
    if (sock < 0) {
        perror("Socket creation failed");
        exit(1);
    }

    // Set socket options
    int opt = 1;
    if (setsockopt(sock, IPPROTO_IP, IP_HDRINCL, &opt, sizeof(opt)) < 0 ||
        setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &opt, sizeof(opt)) < 0) {
        perror("setsockopt failed");
        close(sock);
        exit(1);
    }

    // Set socket to non-blocking mode
    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);

    // Initialize server tracking array
    memset(servers, 0, sizeof(servers));
    
    // Get our local IP
    char *local_ip = get_local_ip();
    local_ip_int = inet_addr(local_ip);
    printf("Client IP: %s\n", local_ip);
    if (debug_mode) {
        printf("Debug mode enabled. Will show all HELLO messages.\n");
    }

    char buffer[BUFFER_SIZE];
    int current_query = 0;
    time_t query_start_time = 0;
    time_t last_discover = 0;
    // Main loop
    while (running) {
        fd_set readfds;
        struct timeval tv;
        int retval;
        
        FD_ZERO(&readfds);
        FD_SET(sock, &readfds);
        
        tv.tv_sec = 0;
        tv.tv_usec = 100000; // 100ms timeout
        
        retval = select(sock + 1, &readfds, NULL, NULL, &tv);
        
        if (retval == -1) {
            if (errno == EINTR) continue; // Interrupted by signal
            perror("select() failed");
            break;
        } else if (retval) {
            // Data is available to read
            struct sockaddr_in src_addr;
            socklen_t addrlen = sizeof(src_addr);
            
            int bytes_received = recvfrom(sock, buffer, BUFFER_SIZE, 0, 
                                         (struct sockaddr *)&src_addr, &addrlen);
            
            if (bytes_received < 0) {
                if (errno != EAGAIN && errno != EWOULDBLOCK) {
                    perror("recvfrom failed");
                }
                continue;
            }
            
            if (bytes_received < sizeof(struct iphdr)) continue;
            
            struct iphdr *iph = (struct iphdr *)buffer;
            
            // Check if it's our protocol
            if (iph->protocol != PROTOCOL_NUM) continue;
            
            // Check if it's our own packet
            if (iph->saddr == local_ip_int) continue;
            
            // Make sure we have enough data for a complete CLDP header
            if (bytes_received < (iph->ihl * 4 + sizeof(struct cldp_header))) continue;
            
            struct cldp_header *cldp = (struct cldp_header *)(buffer + (iph->ihl * 4));
            
            // Now process based on message type
            if (cldp->msg_type == 0x01) {
                // HELLO message
                handle_hello(iph, cldp);
            } 
            else if (cldp->msg_type == 0x03) {
                // RESPONSE message
                if (bytes_received < (iph->ihl * 4 + sizeof(struct cldp_header) + cldp->payload_len)) {
                    continue; // Incomplete packet
                }
                
                char *payload = (char *)(buffer + (iph->ihl * 4) + sizeof(struct cldp_header));
                // Null-terminate the payload for safety
                payload[cldp->payload_len] = '\0';
                
                // Only process responses for our queries
                handle_response(iph, cldp, payload);
            }
        }
        
        // Send HELLO periodically
        time_t now = time(NULL);
        
        // Check if we need to send a new query
        if (current_query > 0) {
            // Check if the current query has timed out
            if (now - query_start_time >= QUERY_TIMEOUT) {
                current_query = 0;
                printf("Query timed out\n");
            }
        }
        
        // Send a discovery query every 30 seconds
        if (now - last_discover >= 30) {
            printf("\nSending discovery broadcast...\n");
            send_query(sock, 1); // 1 = Query for hostname
            last_discover = now;
        }
        
        // Handle user input
        int c = getchar();
        if (c != EOF) {
            switch (c) {
                case 'q':
                case 'Q':
                    running = 0;
                    break;
                case 'l':
                case 'L':
                    print_servers();
                    break;
                case '1':
                    printf("Querying for hostname...\n");
                    send_query(sock, 1);
                    current_query = 1;
                    query_start_time = now;
                    break;
                case '2':
                    printf("Querying for system time...\n");
                    send_query(sock, 2);
                    current_query = 2;
                    query_start_time = now;
                    break;
                case '3':
                    printf("Querying for CPU load...\n");
                    send_query(sock, 3);
                    current_query = 3;
                    query_start_time = now;
                    break;
                case 'h':
                case 'H':
                case '?':
                    printf("\nAvailable commands:\n");
                    printf("  l - List active servers\n");
                    printf("  1 - Query hostname\n");
                    printf("  2 - Query system time\n");
                    printf("  3 - Query CPU load\n");
                    printf("  q - Quit\n");
                    printf("  h - Show this help\n");
                    break;
            }
        }
    }
    
    close(sock);
    printf("Client terminated.\n");
    return 0;
}