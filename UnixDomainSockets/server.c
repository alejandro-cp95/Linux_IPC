#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#define SOCKET_NAME "/tmp/DemoSocket"

int main(int args, char* argv[])
{
    /* In case the program exited all of the sudden leaving behind the socket,
    remove it */
    unlink(SOCKET_NAME);
    int connection_socket;
    connection_socket = socket(AF_UNIX,SOCK_STREAM,0);
    if(connection_socket == -1)
    {
        perror("Socket");
        exit(EXIT_FAILURE);
    }
    printf("Master socket created\n");
    /* Unix domain socket structure */
    struct sockaddr_un name;
    // /* Initialize */
    memset(&name, 0, sizeof(struct sockaddr_un));
    name.sun_family = AF_UNIX;
    strcpy(name.sun_path, SOCKET_NAME);
    int ret = bind(connection_socket, (const struct sockaddr*) &name, sizeof(struct sockaddr_un));
    if(ret == -1)
    {
        perror("Bind");
        exit(EXIT_FAILURE);
    }
    printf("bind() call succeeded\n");
    ret = listen(connection_socket, 20);
    if(ret == -1)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    while(1)
    {
        printf("Waiting on accept system call\n");
        int data_socket = accept(connection_socket,NULL,NULL);
        if(data_socket == -1)
        {
            perror("accept");
            exit(EXIT_FAILURE);
        }
        printf("Connection request from client accepted\n");
        int result = 0;
        while(1)
        {
            int data;
            ret = recv(data_socket,&data,sizeof(int),0);
            if(ret == -1)
            {
                perror("recv");
                exit(EXIT_FAILURE);
            }
            if(data == 0)
            {
                break;
            }
            else
            {
                result += data;
            }
        }
        printf("Sending result\n");
        ret = send(data_socket,&result,sizeof(int),0);
        if(ret == -1)
        {
            perror("send");
            exit(EXIT_FAILURE);
        }
        close(data_socket);
        printf("Connection to client closed\n");
    }
    close(connection_socket);
    printf("Server closed\n");
    unlink(SOCKET_NAME);
    exit(EXIT_SUCCESS);
}