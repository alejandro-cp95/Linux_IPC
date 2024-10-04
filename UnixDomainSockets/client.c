#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#define SOCKET_NAME "/tmp/DemoSocket"

int main(int args, char* argv[])
{
    int data_socket;
    data_socket = socket(AF_UNIX,SOCK_STREAM,0);
    if(data_socket == -1)
    {
        perror("Socket");
        exit(EXIT_FAILURE);
    }
    printf("Client socket created\n");
    struct sockaddr_un addr;
    addr.sun_family = AF_UNIX;
    memcpy(&addr.sun_path,SOCKET_NAME,sizeof(SOCKET_NAME));
    int ret = connect(data_socket,(const struct sockaddr*) &addr, sizeof(struct sockaddr_un));
    if(ret == -1)
    {
        perror("connect");
        exit(EXIT_FAILURE);
    }
    printf("Connection request to server accepted\n");
    int result = 0;
    while(1)
    {
        int data;
        printf("Input a number\n");
        scanf("%d", &data);
        printf("Sending data\n");
        ret = send(data_socket,&data,sizeof(int),0);
        if(ret == -1)
        {
            perror("send");
            exit(EXIT_FAILURE);
        }
        if(data == 0)
        {
            break;
        }
    }
    ret = recv(data_socket,&result,sizeof(int),0);
    if(ret == -1)
    {
        perror("recv");
        exit(EXIT_FAILURE);
    }
    printf("Result is %d\n",result);
    close(data_socket);
    printf("Client closed\n");
    exit(EXIT_SUCCESS);
}