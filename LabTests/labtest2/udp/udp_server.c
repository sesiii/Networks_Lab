#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<error.h>
#include<string.h>

#define port 8080
#define BUFFER_SIZE 1024

int main()
{
    int sock_fd;
    struct sockaddr_in server_addr,client_addr;
    char buffer[BUFFER_SIZE];
    socklen_t addr_len=sizeof(client_addr);

    sock_fd=socket(AF_INET,SOCK_DGRAM,0);
    if(sock_fd<0){
        perror("Socket creation failed.\n");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr,0,sizeof(server_addr));
    server_addr.sin_port=htons(port);
    server_addr.sin_family=AF_INET;
    server_addr.sin_addr.s_addr=INADDR_ANY;

    if(bind(sock_fd,(const struct sockaddr*)&server_addr,sizeof(server_addr))<0){
        perror("Bind failed.\n");
        exit(EXIT_FAILURE);
    }

    printf("UDP server listening on port %d\n",port);

    int n=recvfrom(sock_fd,buffer,BUFFER_SIZE,0,(struct sockaddr *)&client_addr,&addr_len);
    buffer[n]='\0';
    printf("Client: %s\n",buffer);
}