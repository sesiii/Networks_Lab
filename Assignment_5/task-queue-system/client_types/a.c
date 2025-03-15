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
    
    // Read response but don't do anything with it
    int bytes_read = read(client_socket, buffer, MAX_BUFFER - 1);
    buffer[bytes_read] = '\0';
    char *response = buffer;
    printf("Received: %s\n", response);

    // printf("");/
    
    while (1) {
        printf("Received task but not responding...\n");
        sleep(1);
        
        int bytes_written = read(client_socket, buffer, strlen(response));
        buffer[bytes_written] = '\0';
        char *response = buffer;
        // printf("Received: %s\n", response);

        if(strcmp(response,"exit")){

            printf("Received: exit\n");
            break;
        }
    }
    
    close(client_socket);
    return 0;
}