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
#include <sys/sysinfo.h>

#define PROTOCOL_NUM 253
#define BUFFER_SIZE 1024
#define HELLO_INTERVAL 10

// Signal flag for clean termination
volatile int running = 1;

struct cldp_header {
    uint8_t msg_type;
    uint8_t payload_len;
    uint16_t trans_id;
    uint32_t reserved;
};

// Global variables
static uint16_t server_trans_id = 0;
uint32_t local_ip_int;
int debug_mode = 1;

// Function prototypes
uint16_t ip_checksum(void *buf, int len);
char *get_local_ip();
void send_hello(int sock);
void send_response(int sock, uint32_t dest_ip, uint16_t trans_id, uint8_t query_type);
void handle_query(int sock, struct iphdr *iph, struct cldp_header *cldp, char *payload);
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

void send_hello(int sock) {
    char buffer[BUFFER_SIZE];
    struct sockaddr_in dest_addr = { .sin_family = AF_INET, .sin_addr.s_addr = inet_addr("255.255.255.255") };
    uint16_t trans_id = server_trans_id++; 
    
    struct iphdr iph = { 
        .version = 4, 
        .ihl = 5, 
        .ttl = 64, 
        .protocol = PROTOCOL_NUM,
        .id = htons(rand() % 65535)
    };
    
    struct cldp_header cldp = { 
        .msg_type = 0x01, 
        .payload_len = 0, 
        .trans_id = htons(trans_id), 
        .reserved = 0 
    };
    
    iph.tot_len = htons(sizeof(iph) + sizeof(cldp));
    iph.saddr = local_ip_int;
    iph.daddr = dest_addr.sin_addr.s_addr;
    iph.check = ip_checksum(&iph, sizeof(iph));

    memcpy(buffer, &iph, sizeof(iph));
    memcpy(buffer + sizeof(iph), &cldp, sizeof(cldp));

    if (sendto(sock, buffer, ntohs(iph.tot_len), 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            perror("Send HELLO failed");
        }
        return;
    }
    
    if (debug_mode) {
        printf("Sent HELLO (Trans ID: %u)\n", trans_id);
    }
}

void send_response(int sock, uint32_t dest_ip, uint16_t trans_id, uint8_t query_type) {
    char buffer[BUFFER_SIZE];
    struct sockaddr_in dest_addr = { .sin_family = AF_INET, .sin_addr.s_addr = dest_ip };
    
    struct iphdr iph = { 
        .version = 4, 
        .ihl = 5, 
        .ttl = 64, 
        .protocol = PROTOCOL_NUM,
        .id = htons(rand() % 65535)
    };
    
    struct cldp_header cldp = { 
        .msg_type = 0x03,  // RESPONSE
        .trans_id = trans_id,
        .reserved = 0 
    };
    
    char payload[BUFFER_SIZE - sizeof(iph) - sizeof(cldp)];
    int payload_len = 0;
    
    // Prepare payload based on query type
    switch (query_type) {
        case 1: { // Hostname
            char hostname[64];
            gethostname(hostname, sizeof(hostname));
            payload_len = snprintf(payload, sizeof(payload), "%s", hostname);
            break;
        }
        case 2: { // System time
            time_t now = time(NULL);
            struct tm *timeinfo = localtime(&now);
            payload_len = strftime(payload, sizeof(payload), "%Y-%m-%d %H:%M:%S", timeinfo);
            break;
        }
        case 3: { // CPU load
            struct sysinfo info;
            sysinfo(&info);
            // Convert load average to human-readable format (it's stored as int with 1 point = 1.0)
            float load_1 = (float)info.loads[0] / (1 << SI_LOAD_SHIFT);
            float load_5 = (float)info.loads[1] / (1 << SI_LOAD_SHIFT);
            float load_15 = (float)info.loads[2] / (1 << SI_LOAD_SHIFT);
            payload_len = snprintf(payload, sizeof(payload), "Load avg: %.2f, %.2f, %.2f", 
                                  load_1, load_5, load_15);
            break;
        }
        default:
            payload_len = snprintf(payload, sizeof(payload), "Unsupported query type: %d", query_type);
    }
    
    // cldp.payload_len = payload_len;
    cldp.payload_len = (uint8_t)payload_len;
    
    iph.tot_len = htons(sizeof(iph) + sizeof(cldp) + payload_len);
    iph.saddr = local_ip_int;
    iph.daddr = dest_ip;
    iph.check = ip_checksum(&iph, sizeof(iph));

    memcpy(buffer, &iph, sizeof(iph));
    memcpy(buffer + sizeof(iph), &cldp, sizeof(cldp));
    memcpy(buffer + sizeof(iph) + sizeof(cldp), payload, payload_len);

    if (sendto(sock, buffer, ntohs(iph.tot_len), 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            perror("Send RESPONSE failed");
        }
        return;
    }
    
    printf("Sent RESPONSE to %s for query type %d (Trans ID: %u)\n", 
           inet_ntoa(*(struct in_addr *)&dest_ip), query_type, ntohs(trans_id));
}

void handle_query(int sock, struct iphdr *iph, struct cldp_header *cldp, char *payload) {
    // uint8_t query_type = *payload;
    uint8_t query_type = (uint8_t)*payload;
    printf("Received QUERY type %d from %s (Trans ID: %u)\n", 
           query_type, inet_ntoa(*(struct in_addr *)&iph->saddr), ntohs(cldp->trans_id));
    
    // Send response
    send_response(sock, iph->saddr, cldp->trans_id, query_type);
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

    // Initialize random seed
    srand(time(NULL));

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
    
    // Get our local IP
    char *local_ip = get_local_ip();
    local_ip_int = inet_addr(local_ip);
    printf("Server IP: %s\n", local_ip);
    
    if (debug_mode) {
        printf("Debug mode enabled. Will show all HELLO messages.\n");
    }
    
    printf("CLDP Server started. Press Ctrl+C to exit.\n");

    char buffer[BUFFER_SIZE];
    time_t last_hello = 0;

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
            // if (iph->saddr == local_ip_int) continue;
            if (iph->saddr == local_ip_int && !debug_mode) continue;
            
            // Make sure we have enough data for a complete CLDP header
            if (bytes_received < (iph->ihl * 4 + sizeof(struct cldp_header))) continue;
            
            struct cldp_header *cldp = (struct cldp_header *)(buffer + (iph->ihl * 4));
            
            // Process based on message type
            if (cldp->msg_type == 0x02) { // QUERY
                if (bytes_received < (iph->ihl * 4 + sizeof(struct cldp_header) + cldp->payload_len)) {
                    continue; // Incomplete packet
                }
                
                char *payload = (char *)(buffer + (iph->ihl * 4) + sizeof(struct cldp_header));
                handle_query(sock, iph, cldp, payload);
            }
        }
        
        // Send HELLO periodically
        time_t now = time(NULL);
        if (now - last_hello >= HELLO_INTERVAL) {
            send_hello(sock);
            last_hello = now;
        }
    }
    
    close(sock);
    printf("Server terminated.\n");
    return 0;
}