#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 8080
#define MAX_BUFFER 1024


int main() {
    const char *server_ip = "127.0.0.1";
    
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }
    
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        perror("inet_pton failed");
        exit(EXIT_FAILURE);
    }
    
    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect failed");
        exit(EXIT_FAILURE);
    }

    char buffer[MAX_BUFFER];
    printf("Connected to server and doing nothing...\n");
    printf("client %d: Connected to server at %s:%d\n", getpid(),server_ip, PORT);
    // Infinite loop to keep connection open
    while (1) {
        int bytes_read=read(client_socket,buffer,MAX_BUFFER-1);
        buffer[bytes_read]='\0';
        
        if(strcmp(buffer,"exit")==0){
            printf("Received: exit\n");
            break;
        }
        sleep(1);
    }
    
    close(client_socket);
    return 0;
}