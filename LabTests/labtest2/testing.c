// #include<stdio.h>
// #include<stdlib.h>
// #include<string.h>
// #include<arpa/inet.h>
// #include<unistd.h>

// #define SERVER_IP "127.0.0.1"
// #define PORT 8080
// #define BUFFER_SIZE 1024

// int main()
// {
//     int sock_fd;
//     struct sockaddr_in server_addr;
//     char buffer[BUFFER_SIZE];
//     socklen_t addr_len=sizeof(server_addr);

//     sock_fd=socket(AF_INET, SOCK_DGRAM,0);
//     if(sock_fd<0){
//         perror("Socket creation failed.\n");
//         exit(EXIT_FAILURE);
//     }

//     memset(&server_addr,0,sizeof(server_addr));
//     server_addr.sin_family=AF_INET;
//     server_addr.sin_port=htons(PORT);
//     inet_pton(AF_INET,SERVER_IP,&server_addr.sin_addr);


// }

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<arpa/inet.h>

#define SERVER_IP "127.0.0.1"
#define port 8080
#define BUFFER_SIZE 1024

int main()
{
    int sock_fd;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    socklen_t addr_len=sizeof(server_addr);

    sock_fd=socket(AF_INET,SOCK_DGRAM,0);
    if(sock_fd<0){
        perror("Socket creation failed.\n");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr,0,sizeof(server_addr));
    server_addr.sin_family=AF_INET;
    server_addr.sin_port=htons(port);
    inet_pton(AF_INET,SERVER_IP,&server_addr.sin_addr);
    

}

//added few changes
//upated scriot in udp client