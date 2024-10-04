#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/select.h>
#include <unistd.h>

#define SOCKET_NAME "/tmp/DemoSocket"

int fds[31];
fd_set readfds;
int results[31];

void initialize_fds()
{
    memset(fds, -1, sizeof(fds));
}

void refresh_fds()
{
    FD_ZERO(&readfds);
    for(int i = 0; i < 30; i++)
    {
        if(fds[i] != -1)
        {
            FD_SET(fds[i], &readfds);
        }
    }
}

void add_fd(int fd)
{
    for(int i = 0; i <= 30; i++)
    {
        if(fds[i] == -1)
        {
            fds[i] = fd;
            results[i] = 0;
            break;
        }
    }
}

void remove_fd(int fd)
{
    for(int i = 0; i <= 30; i++)
    {
        if(fds[i] == fd)
        {
            fds[i] = -1;
            break;
        }
    }
}

int max_fd()
{
    int retVal = -1;
    for(int i = 0; i <= 30; i++)
    {
        if(fds[i] > retVal)
        {
            retVal = fds[i];
        }
    }
    return retVal;
}

int main(int args, char* argv[])
{
    /* In case the program exited all of the sudden leaving behind the socket,
    remove it */
    unlink(SOCKET_NAME);
    initialize_fds();
    int connection_socket;
    connection_socket = socket(AF_UNIX,SOCK_STREAM,0);
    if(connection_socket == -1)
    {
        perror("Socket");
        exit(EXIT_FAILURE);
    }
    add_fd(connection_socket);
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
        perror("bind");
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
        refresh_fds();
        printf("Waiting on select system call\n");
        ret = select(max_fd() + 1,&readfds, NULL, NULL, NULL);
        if(ret == -1)
        {
            perror("select");
            exit(EXIT_FAILURE);
        }
        if(FD_ISSET(connection_socket, &readfds))
        {
            int data_socket = accept(connection_socket,NULL,NULL);
            if(data_socket == -1)
            {
                perror("accept");
                exit(EXIT_FAILURE);
            }
            printf("Connection request from client accepted\n");
            add_fd(data_socket);
        }
        else
        {
            for(int i = 0; i <= 30; i++)
            {
                if(FD_ISSET(fds[i],&readfds))
                {
                    int data;
                    ret = recv(fds[i],&data,sizeof(int),0);
                    if(ret == -1)
                    {
                        perror("recv");
                        exit(EXIT_FAILURE);
                    }
                    if(data == 0)
                    {
                        printf("Sending result\n");
                        ret = send(fds[i],&results[i],sizeof(int),0);
                        if(ret == -1)
                        {
                            perror("send");
                            exit(EXIT_FAILURE);
                        }
                        close(fds[i]);
                        remove_fd(fds[i]);
                        printf("Connection to client closed\n");
                    }
                    else
                    {
                        results[i] += data;
                    }
                    break;
                }
            }
        }
    }
    close(connection_socket);
    printf("Server closed\n");
    unlink(SOCKET_NAME);
    exit(EXIT_SUCCESS);
}