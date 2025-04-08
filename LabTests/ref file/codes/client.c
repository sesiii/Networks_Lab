#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 8080
#define SERVER_IP "127.0.0.1"
#define BUFFER_SIZE 128

// Struct for sending complex data
typedef struct {
    int id;
    float value;
    char name[16];
} packet_t __attribute__((packed));

// Setup socket for client
int setup_socket(struct sockaddr_in *server_addr) {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        exit(1);
    }

    server_addr->sin_family = AF_INET;
    server_addr->sin_port = htons(PORT);
    inet_pton(AF_INET, SERVER_IP, &server_addr->sin_addr);

    return sockfd;
}

int main() {
    struct sockaddr_in server_addr;
    int sockfd = setup_socket(&server_addr);
    socklen_t addr_len = sizeof(server_addr);

    // 1. Send a String
    char buffer[50]; 
sprintf(buffer, "HI I am a boy... %d sp what", 10);
printf("%s\n", buffer);
    sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr*)&server_addr, addr_len);
    printf("Sent string: %s\n", buffer);
    sleep(1); // Delay for server to process

    // 2. Send an Integer (binary)
    int int_num = htonl(42);
    sendto(sockfd, &int_num, sizeof(int_num), 0, (struct sockaddr*)&server_addr, addr_len);
    printf("Sent integer: %d\n", ntohl(int_num));
    sleep(1);

    // 3. Send a Float (binary, approximated)
    float float_num = 3.14;
    uint32_t net_float;
    memcpy(&net_float, &float_num, sizeof(float));
    net_float = htonl(net_float);
    sendto(sockfd, &net_float, sizeof(net_float), 0, (struct sockaddr*)&server_addr, addr_len);
    printf("Sent float: %.2f\n", float_num);
    sleep(1);

    // 4. Send a Struct
    packet_t packet = { .id = htonl(123), .value = 45.67, .name = "Test Packet" };
    uint32_t net_value;
    memcpy(&net_value, &packet.value, sizeof(float));
    net_value = htonl(net_value);
    char struct_buffer[sizeof(packet_t)];
    memcpy(struct_buffer, &packet.id, sizeof(int));
    memcpy(struct_buffer + sizeof(int), &net_value, sizeof(uint32_t));
    memcpy(struct_buffer + sizeof(int) + sizeof(uint32_t), packet.name, sizeof(packet.name));
    sendto(sockfd, struct_buffer, sizeof(packet_t), 0, (struct sockaddr*)&server_addr, addr_len);
    printf("Sent struct: id=%d, value=%.2f, name=%s\n", ntohl(packet.id), packet.value, packet.name);
    sleep(1);

    // 5. Send Binary Data
    unsigned char binary_data[] = {0xDE, 0xAD, 0xBE, 0xEF};
    sendto(sockfd, binary_data, sizeof(binary_data), 0, (struct sockaddr*)&server_addr, addr_len);
    printf("Sent binary: ");
    for (size_t i = 0; i < sizeof(binary_data); i++) printf("%02x ", binary_data[i]);
    printf("\n");
    sleep(1);

    // 6. Send Combined Data (int, float, string)
    int comb_int = htonl(99);
    float comb_float = 2.718;
    const char *comb_string = "Combined";
    uint32_t net_comb_float;
    memcpy(&net_comb_float, &comb_float, sizeof(float));
    net_comb_float = htonl(net_comb_float);
    char comb_buffer[BUFFER_SIZE];
    size_t offset = 0;
    memcpy(comb_buffer + offset, &comb_int, sizeof(comb_int)); offset += sizeof(comb_int);
    memcpy(comb_buffer + offset, &net_comb_float, sizeof(net_comb_float)); offset += sizeof(net_comb_float);
    memcpy(comb_buffer + offset, comb_string, strlen(comb_string)); offset += strlen(comb_string);
    sendto(sockfd, comb_buffer, offset, 0, (struct sockaddr*)&server_addr, addr_len);
    printf("Sent combined: int=%d, float=%.3f, string=%s\n", ntohl(comb_int), comb_float, comb_string);

    close(sockfd);
    printf("Client finished sending all data.\n");
    return 0;
}