#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<arpa/inet.h>

#define port 8080
#define BUFFER_SIZE 1024

int main()
{
    int sock_fd=socket(AF_INET,SOCK_DGRAM,0);
    if(sock_fd<0){
        //
    }
    struct sockaddr_in client_addr,server_addr;
    socklen_t addr_len=sizeof(client_addr);
    server_addr.sin_addr.s_addr=INADDR_ANY;
    server_addr.sin_family=AF_INET;
    server_addr.sin_port=htons(port);

    if(bind(sock_fd,(struct sockaddr*)&server_addr,sizeof(server_addr))<0){
        //
        perror("Bind failed.\n");
        exit(EXIT_FAILURE);
    }
    char buffer[BUFFER_SIZE];
    printf("Udp server running on port %d\n",port);
    int n=recvfrom(sock_fd,buffer,BUFFER_SIZE,0,(struct sockaddr *)&client_addr,&addr_len);
    buffer[n]='\0';
    printf("Client: %s\n",buffer);
}