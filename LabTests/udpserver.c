#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<arpa/inet.h>
#include<unistd.h>

#define port 8080
#define buffer_size 1024

int main()
{
    int sock_fd;
    struct sockaddr_in client_addr,server_addr;
    char buffer[buffer_size];
    socklen_t addr_len=sizeof(client_addr);

    sock_fd=socket(AF_INET,SOCK_DGRAM,0);
    if(sock_fd<0){
        perror("Socket creation failed.\n");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr,0,sizeof(server_addr));
    server_addr.sin_addr.s_addr=INADDR_ANY;
    server_addr.sin_family=AF_INET;
    server_addr.sin_port=htons(port);

    if(bind(sock_fd,(struct sockaddr*)&server_addr,sizeof(server_addr))<0){
        perror("Binding failed.\n");
        exit(EXIT_FAILURE);
    }

    printf("UDP server running on port: %d\n\n",port);

    int n=recvfrom(sock_fd,buffer,buffer_size,0,( struct sockaddr*)&client_addr,&addr_len);
    buffer[n]='\0';
    // sleep(1);
    printf("Client:\n%s",buffer);

    char *msg="Heyy client!\nI just got your message..\n";
    sendto(sock_fd,msg,strlen(msg),0,( struct sockaddr*)&client_addr,addr_len);

    // sleep(1);
    printf("Quitting...");
    close(sock_fd);
    return 0;


}