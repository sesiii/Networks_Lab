#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<arpa/inet.h>
#include<unistd.h>

#define port 8080
#define buffer_size 1024

int main()
{
    int client_fd,server_fd;
    struct sockaddr_in server_addr,client_addr;
    char buffer[buffer_size];
    socklen_t client_len=sizeof(client_addr);

    server_fd=socket(AF_INET,SOCK_STREAM,0);
    if(server_fd<0){
        perror("Socket creation failed.\n");
        exit(EXIT_FAILURE);
    }
    memset(&server_addr,0,sizeof(server_addr));
    server_addr.sin_addr.s_addr=htonl(INADDR_ANY);
    server_addr.sin_family=AF_INET;
    server_addr.sin_port=htons(port);

    if(bind(server_fd,(const struct sockaddr*)&server_addr,sizeof(server_addr))<0){
        perror("Binding failed.\n");
        exit(EXIT_FAILURE);
    }

    if(listen(server_fd,5)<0){
        perror("Listen failed.\n");
        exit(EXIT_FAILURE);
    }

    printf("TCP server listening on port: %d\n",port);

    client_fd=accept(server_fd,(struct sockaddr*)&client_addr,&client_len);
    if(client_fd<0){
        perror("Client accept failed.\n");
        exit(EXIT_FAILURE);
    }

    printf("Client connected.\n");

    int n=read(client_fd,buffer,buffer_size);
    buffer[n]='\0';
    printf("Client:\n%s",buffer);

    char *ack_msg="Heyy client! Just recieved your message...\n";
    write(client_fd,ack_msg,strlen(ack_msg));

    close(client_fd);
    close(server_fd);
    return 0;

}