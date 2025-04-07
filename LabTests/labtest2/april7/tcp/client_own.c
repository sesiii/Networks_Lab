#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>

#define PORT 8080

int main() {
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        perror("Socket creation failed");
        exit(1);
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_port = htons(PORT);

    if (connect(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connect failed");
        exit(1);
    }

    srand(time(NULL));
    int a_ = rand() % 100;
    write(sock_fd, &a_, sizeof(int));
    printf("Sent %d\n", a_);

    int b_;
    int n = read(sock_fd, &b_, sizeof(int));
    if (n > 0) {
        printf("Received %d\n", b_);
    } else {
        printf("No response received\n");
    }

    close(sock_fd);
    return 0;
}