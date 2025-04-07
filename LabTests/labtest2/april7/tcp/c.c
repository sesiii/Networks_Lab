// #include<stdio.h>
// #include<string.h>
// #include<stdlib.h>
// #include<unistd.h>
// #include<arpa/inet.h>
// #include<signal.h>

// #define port 8080
// #define BUFFER_SIZE 1024

// int main()
// {
//     int sock_fd=socket(AF_INET,SOCK_STREAM,0);
//     if(sock_fd<0){
//         //
//     }

//     struct sockaddr_in server_addr;
//     server_addr.sin_addr.s_addr=inet_addr("127.0.0.1");
//     server_addr.sin_family=AF_INET;
//     server_addr.sin_port=htons(port);

//     if(connect(sock_fd,(struct sockaddr*)&server_addr,sizeof(server_addr))<0){
//         //
//     }
//     char *str="ho anna..\n";
//     write(sock_fd,str,strlen(str));

//     close(sock_fd);
//     return 0;
// }


#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<unistd.h>
#include<arpa/inet.h>

#define port 8080
#define BUFFER_SIZE 1024

int main()
{
    int sock_fd=socket(AF_INET,SOCK_STREAM,0);
    if(sock_fd<0){
        //
    }

    struct sockaddr_in server_addr;
    server_addr.sin_addr.s_addr=inet_addr("127.0.0.1");
    server_addr.sin_family=AF_INET;
    server_addr.sin_port=htons(port);

    if(connect(sock_fd,(struct sockaddr*)&server_addr,sizeof(server_addr))<0){
        //
    }
    srand(time(NULL));
    int a = rand() % 100;
    int b=rand()%100;
    int c=a+b;
    char ab[100];
    sprintf(ab,"sum of %d and %d is %d\n",a,b,c);
    char *str="hi ra server_bhai. lmao";
    write(sock_fd,ab,strlen(ab));
    close(sock_fd);
    return 0;
}