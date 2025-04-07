#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>

#define port 8080
#define BUFFER_SIZE 1024
int su(int a,int b){
    return a+b;
}
void handle_client(int client_sock,char *client_ip,int client_port){
    while(1){
    char buffer[BUFFER_SIZE];
    int a_,b_;
    int a=read(client_sock,&a_,sizeof(int));
    printf("Received from %s:%d--> %d\n",client_ip,client_port,a_);
    write(client_sock,"ACK",3);
    int b=read(client_sock,&b_,sizeof(int));
    printf("Received from %s:%d--> %d\n",client_ip,client_port,b_);
    write(client_sock,"ACK",3);
    int c=su(a_,b_);
    printf("%d\n",c);
    write(client_sock,&c,sizeof(int));
    printf("Sent sum of %d and %d: %d\n",a_,b_,c);
    }
    close(client_sock);

    exit(0);

}
int main()
{
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0)
    {
        //
        perror("Socket creation failed.\n");
        exit(EXIT_FAILURE);
    }
    struct sockaddr_in client_addr, server_addr;
    socklen_t addr_len = sizeof(client_addr);
    signal(SIGCHLD, SIG_IGN);

    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if (bind(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        //
        perror("bind failed.\n");
        exit(EXIT_FAILURE);
    }
    if (listen(sock_fd, 10) < 0)
    {
        //
        perror("listen failed.\n");
        exit(EXIT_FAILURE);
    }

    printf("Server running on port %d\n", port);

    while (1)
    {
        int client_sock = accept(sock_fd, (struct sockaddr *)&client_addr, &addr_len);
        if (client_sock < 0)
        {
            //
            perror("client_sock failed.\n");
        exit(EXIT_FAILURE);
        }
        char client_ip[16];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, 16);
        int client_port = ntohs(client_addr.sin_port);
        printf("Client connected from %s:%d\n", client_ip, client_port);

        pid_t pid = fork();
        if (pid == 0)
        {
            close(sock_fd);
            handle_client(client_sock,client_ip,client_port);
        }
        else if (pid > 0)
        {
            close(client_sock);
        }
        else
        {
            perror("Fork failed.\n");
        }
    }
    close(sock_fd);
    return 0;
}