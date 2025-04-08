//Name: Dadi Sasank Kumar
//Rno: 22CS10020

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<arpa/inet.h>


#define buffer_size 1024
#define port 8080
#define server_ip "127.0.0.1"


int main()
{
    int sock_fd;
    struct sockaddr_in server_addr;
    char buffer[buffer_size];
    // char ip[buffer_size];
    // int port_no;
    // int round_no;

    socklen_t server_len=sizeof(server_addr);

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

    char *IP="127.0.0.1";
    char *port_no="8080";

    // char *initial=IP+port;
    // printf("%c\n",initial);
    

    write(sock_fd,IP,strlen(IP));
    printf("IP Address sent\n");
    sleep(1);

    
    write(sock_fd,port_no,strlen(port_no));
    printf("Port sent\n");
    sleep(1);

    int n=read(sock_fd,buffer,buffer_size);
    buffer[n]='\0';
    char *roundnoquestion=buffer;
    printf("%s ",roundnoquestion);

    // int p=read(sock_fd,buffer,buffer_size);
    // buffer[p]='\0';
    // char *round_no=buffer;
    // printf("%s",buffer);
    // printf("%s %s\n",roundnoquestion,roundno);
    
    int m=rand()%10;
    printf("\n%s : %s Number %d sent to server.\n",IP,port_no,m);
    write(sock_fd,&m,sizeof(int));

    close(sock_fd);
    return 0;

}
