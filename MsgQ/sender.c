#include <stdio.h>
#include <stdlib.h>
#include <mqueue.h>
#include <string.h>
#include <errno.h>

#define MAX_MSG_SIZE 20
#define SENDER_BUFFER_SIZE (MAX_MSG_SIZE + 1)

int main(int argc, char** argv)
{
    int ret;
    if(argc != 2)
    {
        perror("Provide a msgQ name : format </msgQ_name>\n");
        exit(EXIT_FAILURE);
    }
    else
    {
        mqd_t messageQueue;
        messageQueue = mq_open(argv[1], O_CREAT | O_WRONLY, 0, 0);
        if(messageQueue == -1)
        {
            perror("mq_open failed\n");
            exit(EXIT_FAILURE);
        }
        char msg[SENDER_BUFFER_SIZE];
        memset(msg, 0, sizeof(msg));
        unsigned int prio = 0;
        printf("Input a message:\n");
        scanf("%s", msg);
        ret = mq_send(messageQueue, msg, strlen(msg), prio);
        if(ret == -1)
        {
            perror("mq_send failed\n");
            exit(EXIT_FAILURE);
        }
        printf("Message sent\n");
        ret = mq_close(messageQueue);
        if(ret == -1)
        {
            perror("mq_close failed\n");
            exit(EXIT_FAILURE);
        }
    }
}