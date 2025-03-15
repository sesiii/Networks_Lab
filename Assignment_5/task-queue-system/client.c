/*
=====================================
Assignment 5 Submission
Name: Your_Name
Roll number: Your_Roll_Number
=====================================
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_BUFFER 1024
#define PORT 8080

// Function to perform arithmetic operation
double calculate(char *expression) {
    double num1, num2;
    char operator;
    
    // Parse the expression
    sscanf(expression, "%lf %c %lf", &num1, &operator, &num2);
    
    // Perform the calculation
    switch (operator) {
        case '+':
            return num1 + num2;
        case '-':
            return num1 - num2;
        case '*':
            return num1 * num2;
        case '/':
            if (num2 == 0) {
                printf("Client %d: Error: Division by zero\n",getpid());
                return 0;
            }
            return num1 / num2;
        default:
            printf("Client %d: Error: Unsupported operator %c\n",getpid(), operator);
            return 0;
    }
}

int main() {
    // if (argc != 2) {
    //     fprintf(stderr, "Usage: %s <server_ip>\n", argv[0]);
    //     exit(EXIT_FAILURE);
    // }

    const char *server_ip = "127.0.0.1";
    
    // Create client socket
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }
    
    // Configure server address
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    
    // Convert IP address from text to binary form
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        perror("inet_pton failed");
        exit(EXIT_FAILURE);
    }
    
    // Connect to the server
    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect failed");
        exit(EXIT_FAILURE);
    }
    
    printf("Client %d: Connected to server at %s:%d\n", getpid(),server_ip, PORT);
    
    char buffer[MAX_BUFFER];
    char send_buffer[MAX_BUFFER];
    
    while (1) {
        // Request a new task
        printf("Client %d: Requesting task from server...\n", getpid());
        strcpy(send_buffer, "GET_TASK");
        write(client_socket, send_buffer, strlen(send_buffer));
        
        // Receive server response
        memset(buffer, 0, MAX_BUFFER);
        int bytes_read = read(client_socket, buffer, MAX_BUFFER - 1);
        
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';
            printf("Server: %s\n", buffer);
            
            // Check if there are no tasks available
            if (strncmp(buffer, "No tasks available", 18) == 0) {
                printf(" Client %d: No more tasks available. Exiting...\n", getpid());
                break;
            }
            
            // Check for error messages
            if (strncmp(buffer, "Error", 5) == 0) {
                printf("Client %d: Received error: %s\n",getpid(), buffer);
                sleep(1);
                continue;
            }
            
            // Process task if received
            if (strncmp(buffer, "Task:", 5) == 0) {
                // Extract the expression (skip "Task: ")
                char expression[MAX_BUFFER];
                strcpy(expression, buffer + 6);
                
                printf("Client %d: Processing task: %s\n",getpid(), expression);
                
                
                usleep(1);
                
                // Calculate the result
                double result = calculate(expression);
                
                sleep(4);
                // Send the result back to the server
                sprintf(send_buffer, "RESULT %.2f", result);
                printf("Client %d: Sending result: %s\n", getpid(),send_buffer);

                write(client_socket, send_buffer, strlen(send_buffer));
                
                // Receive acknowledgment
                memset(buffer, 0, MAX_BUFFER);
                bytes_read = read(client_socket, buffer, MAX_BUFFER - 1);
                
                if (bytes_read > 0) {
                    buffer[bytes_read] = '\0';
                    printf("Client %d: Server: %s\n",getpid(), buffer);
                }
                
                // Wait a moment before requesting the next task
                sleep(1);
            }
        } else if (bytes_read == 0) {
            printf("Client %d:Server disconnected\n",getpid());
            break;
        } else {
            perror(" read error");
            break;
        }
    }
    
    // Send exit message to server
    strcpy(send_buffer, "exit");
    write(client_socket, send_buffer, strlen(send_buffer));
    
    // Close the socket
    close(client_socket);
    
    return 0;
}