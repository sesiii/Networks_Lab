#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 8080
#define BUFFER_SIZE 128

// Struct for receiving complex data
typedef struct {
    int id;
    float value;
    char name[16];
} packet_t __attribute__((packed));

// Setup socket for server
int setup_socket(struct sockaddr_in *server_addr) {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        exit(1);
    }

    server_addr->sin_family = AF_INET;
    server_addr->sin_addr.s_addr = INADDR_ANY;
    server_addr->sin_port = htons(PORT);
    if (bind(sockfd, (struct sockaddr*)server_addr, sizeof(*server_addr)) < 0) {
        perror("Bind failed");
        exit(1);
    }

    return sockfd;
}

int main() {
    struct sockaddr_in server_addr, client_addr;
    int sockfd = setup_socket(&server_addr);
    socklen_t addr_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];

    printf("UDP Server listening on port %d...\n", PORT);
    while (1) {
        ssize_t n = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr*)&client_addr, &addr_len);
        if (n < 0) {
            perror("Recvfrom failed");
            continue;
        }

        printf("Received %zd bytes from %s:%d: ", n, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        // Interpret based on size and content
        if (n == strlen("Hello, UDP Server!")) { // String
            buffer[n] = '\0';
            printf("String: %s\n", buffer);
        } else if (n == sizeof(int)) { // Integer
            int num;
            memcpy(&num, buffer, sizeof(int));
            printf("Integer: %d\n", ntohl(num));
        } else if (n == sizeof(uint32_t)) { // Float (approximated)
            uint32_t net_float;
            memcpy(&net_float, buffer, sizeof(uint32_t));
            net_float = ntohl(net_float);
            float float_num;
            memcpy(&float_num, &net_float, sizeof(float));
            printf("Float: %.2f\n", float_num);
        } else if (n == sizeof(packet_t)) { // Struct
            int id;
            uint32_t net_value;
            char name[16];
            memcpy(&id, buffer, sizeof(int));
            memcpy(&net_value, buffer + sizeof(int), sizeof(uint32_t));
            memcpy(name, buffer + sizeof(int) + sizeof(uint32_t), sizeof(name));
            float value;
            net_value = ntohl(net_value);
            memcpy(&value, &net_value, sizeof(float));
            printf("Struct: id=%d, value=%.2f, name=%s\n", ntohl(id), value, name);
        } else if (n == 4 && memcmp(buffer, "\xDE\xAD\xBE\xEF", 4) == 0) { // Binary
            printf("Binary: ");
            for (ssize_t i = 0; i < n; i++) printf("%02x ", (unsigned char)buffer[i]);
            printf("\n");
        } else if (n > sizeof(int) + sizeof(uint32_t)) { // Combined data
            int comb_int;
            uint32_t net_comb_float;
            char comb_string[BUFFER_SIZE];
            size_t offset = 0;
            memcpy(&comb_int, buffer, sizeof(int)); offset += sizeof(int);
            memcpy(&net_comb_float, buffer + offset, sizeof(uint32_t)); offset += sizeof(uint32_t);
            memcpy(comb_string, buffer + offset, n - offset);
            comb_string[n - offset] = '\0';
            float comb_float;
            net_comb_float = ntohl(net_comb_float);
            memcpy(&comb_float, &net_comb_float, sizeof(float));
            printf("Combined: int=%d, float=%.3f, string=%s\n", ntohl(comb_int), comb_float, comb_string);
        } else {
            printf("Unknown data: ");
            for (ssize_t i = 0; i < n; i++) printf("%02x ", (unsigned char)buffer[i]);
            printf("\n");
        }
    }

    close(sockfd); // Unreachable in this loop, but included for completeness
    return 0;
}