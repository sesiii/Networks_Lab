#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_BUFFER 1024
#define PORT 8080

int main() {
    const char *server_ip = "127.0.0.1";
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, server_ip, &server_addr.sin_addr);
    
    connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr));
    
    char buffer[MAX_BUFFER];
    strcpy(buffer, "GET_TASK");
    write(client_socket, buffer, strlen(buffer));
    
    memset(buffer, 0, MAX_BUFFER);
    read(client_socket, buffer, MAX_BUFFER - 1);
    printf("Received task: %s\n", buffer);
    printf("Doing nothing with the task...\n");
    
    while (1) {
        sleep(1);
    }
    
    close(client_socket);
    return 0;
}