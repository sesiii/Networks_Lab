

// Assignment 6 Submission
// Name: Dadi Sasank Kumar
// Roll number: 22CS10020

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define BUFFER_SIZE 2048

void read_response(int sock_fd, char *response, int expect_multiline);

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("Usage: %s <server_ip> <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int sock_fd;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    char response[BUFFER_SIZE];
    int port = atoi(argv[2]);

    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, argv[1], &server_addr.sin_addr) <= 0)
    {
        printf("Invalid IP address: %s\n", argv[1]);
        close(sock_fd);
        exit(EXIT_FAILURE);
    }

    printf("Attempting to connect to %s:%d\n", argv[1], port);
    if (connect(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Connection to server failed");
        close(sock_fd);
        exit(EXIT_FAILURE);
    }
    printf("Connected to My_SMTP server at %s:%d\n", argv[1], port);

    while (1)
    {
        printf("> ");
        fflush(stdout);
        if (fgets(buffer, BUFFER_SIZE, stdin) == NULL)
            break;

        if (strlen(buffer) <= 1)
            continue; 

        write(sock_fd, buffer, strlen(buffer));

        if (strncmp(buffer, "DATA", 4) == 0)
        {
            read_response(sock_fd, response, 0); 
            printf("%s", response);
            if (strstr(response, "Enter your message") != NULL)
            {
                // printf("Enter your message (end with a single dot '.'):\n");
                while (1)
                {
                    fgets(buffer, BUFFER_SIZE, stdin);
                    write(sock_fd, buffer, strlen(buffer));
                    if (strcmp(buffer, ".\n") == 0)
                        break;
                }
                read_response(sock_fd, response, 0); 
                printf("%s", response);
            }
        }
        else if (strncmp(buffer, "LIST", 4) == 0 || strncmp(buffer, "GET_MAIL", 8) == 0)
        {
            read_response(sock_fd, response, 1); 
            printf("%s", response);
        }
        else
        {
            read_response(sock_fd, response, 0); 
            printf("%s", response);
        }
    }

    close(sock_fd);
    return 0;
}

void read_response(int sock_fd, char *response, int expect_multiline)
{
    int total_read = 0;
    int complete = 0;

    memset(response, 0, BUFFER_SIZE); // Clear response buffer

    while (!complete)
    {
        int n = read(sock_fd, response + total_read, BUFFER_SIZE - total_read - 1);
        if (n <= 0)
        {
            printf("Server disconnected\n");
            close(sock_fd);
            exit(0);
        }
        total_read += n;
        response[total_read] = '\0';

        // Check for complete response
        if (expect_multiline)
        {
            if (strstr(response, "\n---\n"))
            {
                complete = 1;
            }
        }
        else
        {
            if (strchr(response, '\n'))
            {
                complete = 1;
            }
        }
    }

    if (strncmp(response, "200 Goodbye", 11) == 0)
    {
        close(sock_fd);
        exit(0);
    }
}