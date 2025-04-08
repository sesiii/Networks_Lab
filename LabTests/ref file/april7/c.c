#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define port 8081
#define BUFFER_SIZE 1024

int main()
{
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0)
    {
        //
        perror("socket creation failed\n");
        exit(EXIT_FAILURE);
    }
    struct sockaddr_in server_addr, client_addr;
    char buffer[BUFFER_SIZE];
    socklen_t addr_len = sizeof(server_addr);

    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if (connect(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        // accept failed
        perror("Connect failed\n");
        exit(EXIT_FAILURE);
    }
    char *hello = "HELLO";
    write(sock_fd, hello, strlen(hello));
    printf("HELLO sent\n");

    while (1)
    {
        int n = read(sock_fd, buffer, BUFFER_SIZE - 1);
        if (n <= 0)
        {
            printf("Server closed the connection.\n");
            break;
        }
        buffer[n] = '\0';
        printf("Server: %s\n", buffer);
    }
    close(sock_fd);
    return 0;
}