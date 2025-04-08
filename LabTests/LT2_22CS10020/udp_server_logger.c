

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<signal.h>

#define PORT 9090
#define BUFFER_SIZE 1024

int main()
{
    int sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_fd < 0) {
        perror("Socket creation failed.\n");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in client_addr, server_addr;
    socklen_t addr_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];

    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_family = AF_INET;

    
    signal(SIGCHLD, SIG_IGN);

    if (bind(sock_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed.\n");
        exit(EXIT_FAILURE);
    }

    FILE *log_file = fopen("udp_server.log", "a");
    if (log_file == NULL) {
        perror("Failed to open log file.\n");
        exit(EXIT_FAILURE);
    }

    printf("UDP server running on port %d\n", PORT);

    while (1) {
        int n = recvfrom(sock_fd, buffer, BUFFER_SIZE, 0, (struct sockaddr*)&client_addr, &addr_len);
        if (n < 0) {
            perror("Recvfrom failed.\n");
            continue;
        }
        
        buffer[n] = '\0'; 
        
        // Get client IP and port
        char client_ip[16];
        int client_port = ntohs(client_addr.sin_port);
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
        
        // Log the received message
        fprintf(log_file, "[Date Time] From %s:%d - %s\n", client_ip, client_port, buffer);
        fflush(log_file);  

        // Respond to the client
        char response[BUFFER_SIZE];
        sprintf(response, "ACKED %s", buffer);
        sendto(sock_fd, response, strlen(response), 0, (struct sockaddr*)&client_addr, addr_len);
    }

    close(sock_fd);
    fclose(log_file);
    return 0;
}
