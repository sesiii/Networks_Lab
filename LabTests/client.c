#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<arpa/inet.h>

#define port 8080
#define server_ip "127.0.0.1"
#define buffer_size 1024

int main()
{
    int sock_fd;
    struct sockaddr_in server_addr;
    char buffer[buffer_size];

    sock_fd=socket(AF_INET,SOCK_STREAM,0);
    if(sock_fd<0){
        perror("Socket creation failed.\n");
        exit(EXIT_FAILURE);
    }
    memset(&server_addr,0,sizeof(server_addr));
    server_addr.sin_family=AF_INET;
    server_addr.sin_port=htons(port);

    inet_pton(AF_INET,server_ip,&server_addr.sin_addr);

    if(connect(sock_fd,(struct sockaddr*)&server_addr,sizeof(server_addr))<0){
        perror("Connection to server failed.\n");
        exit(EXIT_FAILURE);
    }

    printf("Connected to server.\n");

    // Step 4: Send an initial message
    const char *msg = "Hello, server!";
    write(sock_fd, msg, strlen(msg));

    int n=read(sock_fd,buffer,buffer_size);
    buffer[n]='\0';
    printf("Server:\n%s",buffer);
    close(sock_fd);
    return 0;


}