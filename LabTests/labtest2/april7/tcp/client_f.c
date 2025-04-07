#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<time.h>

#define port 8080
#define BUFFER_SIZE 1024

int main()
{
    int sock_fd=socket(AF_INET,SOCK_STREAM,0);
    if(sock_fd<0){
        //
        perror("Socket creation failed.\n");
        exit(EXIT_FAILURE);
    }
    char buffer[BUFFER_SIZE];
    struct sockaddr_in server_addr;
    socklen_t addr_len=sizeof(server_addr);
    
    server_addr.sin_addr.s_addr=inet_addr("127.0.0.1");
    server_addr.sin_family=AF_INET;
    server_addr.sin_port=htons(port);

    if(connect(sock_fd,(struct sockaddr*)&server_addr,sizeof(server_addr))<0){
        //
        perror("connect failed.\n");
        exit(EXIT_FAILURE);
    }
    srand(time(NULL));
    while(1){
        // sleep(2);
    int a=rand()%100, b=rand()%100;
    int sum_;
    char inte[10];
    printf("a\n");
    write(sock_fd,&a,sizeof(int));
    int n=read(sock_fd,buffer,BUFFER_SIZE);
    buffer[n]='\0';
    printf("b\n");
    write(sock_fd,&b,sizeof(int));
    int m=read(sock_fd,buffer,BUFFER_SIZE);
    buffer[m]='\0';
    printf("Waiting for sum of numbers %d and %d\n",a,b);
    int o=read(sock_fd,&sum_,sizeof(int));
    printf("Received: %d\n",sum_);
    }
    close(sock_fd);

}