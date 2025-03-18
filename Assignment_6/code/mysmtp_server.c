#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <time.h>

#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024
#define MAILBOX_DIR "mailbox/"

void *handle_client(void *arg);
void process_command(int client_sock, char *command);
void store_email(char *recipient, char *sender, char *body);

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <port>\n", argv[0]);
        return 1;
    }

    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    int port = atoi(argv[1]);

    // Create socket
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        perror("Socket creation failed");
        return 1;
    }

    // Configure server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    // Bind and listen
    if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        return 1;
    }
    if (listen(server_sock, MAX_CLIENTS) < 0) {
        perror("Listen failed");
        return 1;
    }

    printf("Listening on port %d...\n", port);
    mkdir(MAILBOX_DIR, 0777);

    while (1) {
        client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &client_len);
        if (client_sock < 0) {
            perror("Accept failed");
            continue;
        }

        printf("Client connected: %s\n", inet_ntoa(client_addr.sin_addr));
        
        pthread_t thread;
        int *client_sock_ptr = malloc(sizeof(int));
        *client_sock_ptr = client_sock;
        pthread_create(&thread, NULL, handle_client, client_sock_ptr);
        pthread_detach(thread);
    }

    close(server_sock);
    return 0;
}

void *handle_client(void *arg) {
    int client_sock = *(int*)arg;
    free(arg);
    char buffer[BUFFER_SIZE];

    while (1) {
        int bytes_received = recv(client_sock, buffer, BUFFER_SIZE - 1, 0);
        if (bytes_received <= 0) {
            printf("Client disconnected\n");
            break;
        }
        
        buffer[bytes_received] = '\0';
        process_command(client_sock, buffer);
    }

    close(client_sock);
    return NULL;
}

void process_command(int client_sock, char *command) {
    char response[BUFFER_SIZE];
    char sender[256], recipient[256], body[BUFFER_SIZE];
    
    if (strncmp(command, "HELO", 4) == 0) {
        sprintf(response, "200 OK\n");
        printf("HELO received from %s\n", command + 5);
        char *welcome_msg = "Welcome to MySMTP server!";
        send(client_sock, welcome_msg, strlen(welcome_msg), 0);
        
    }
    else if (strncmp(command, "MAIL FROM:", 10) == 0) {
        sscanf(command, "MAIL FROM: %s", sender);
        sprintf(response, "200 OK\n");
        printf("MAIL FROM: %s\n", sender);
    }
    else if (strncmp(command, "RCPT TO:", 8) == 0) {
        sscanf(command, "RCPT TO: %s", recipient);
        sprintf(response, "200 OK\n");
        printf("RCPT TO: %s\n", recipient);
    }
    else if (strcmp(command, "DATA\n") == 0) {
        sprintf(response, "200 OK\n");
        printf("DATA received\n");
        // In a real implementation, you'd collect the body here
    }
    else if (strncmp(command, "LIST", 4) == 0) {
        // Implement email listing
        sprintf(response, "200 OK\n");
    }
    else if (strncmp(command, "GET_MAIL", 8) == 0) {
        // Implement email retrieval
        sprintf(response, "200 OK\n");
    }
    else if (strcmp(command, "QUIT\n") == 0) {
        sprintf(response, "200 Goodbye\n");
    }
    else {
        sprintf(response, "400 ERR\n");
    }

    send(client_sock, response, strlen(response), 0);
}