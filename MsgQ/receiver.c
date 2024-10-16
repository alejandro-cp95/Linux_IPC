#include <stdio.h>
#include <stdlib.h>
#include <mqueue.h>
#include <sys/select.h>
#include <string.h>
#include <errno.h>

#define Q_PERMISSIONS 0660
#define RCVR_BUFFER_SIZE 20

fd_set set;

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
        struct mq_attr attributes;
        attributes.mq_maxmsg = 10;
        attributes.mq_msgsize = RCVR_BUFFER_SIZE;
        attributes.mq_flags = 0;
        attributes.mq_curmsgs = 0;
        messageQueue = mq_open(argv[1], O_CREAT | O_RDONLY, Q_PERMISSIONS, &attributes);
        if(messageQueue == -1)
        {
            perror("mq_open failed\n");
            exit(EXIT_FAILURE);
        }
        while(1)
        {
            FD_ZERO(&set);
            FD_SET(messageQueue, &set);
            printf("Waiting for message\n");
            select(messageQueue + 1, &set, NULL, NULL, NULL);
            if(FD_ISSET(messageQueue, &set))
            {
                char msg[RCVR_BUFFER_SIZE];
                unsigned int prio;
                mq_receive(messageQueue, msg, sizeof(msg), &prio);
                printf("Message: %s\n Priority: %d\n", msg, prio);
            }
        }
        ret = mq_close(messageQueue);
        if(ret == -1)
        {
            perror("mq_close failed\n");
            exit(EXIT_FAILURE);
        }
        ret = mq_unlink(argv[1]);
        if(ret == -1)
        {
            perror("mq_unlink failed\n");
            exit(EXIT_FAILURE);
        }
    }
}