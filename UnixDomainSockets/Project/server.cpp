#include <iostream>
#include <string.h>
#include <filesystem>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/select.h>
#include <unistd.h>
#include <unordered_set>
#include <unordered_map>
#include <array>
#include "project_socket.h"

using namespace std;

unordered_set<int> fds;
fd_set readfds;
unordered_map<array<char, 17>, pair<array<char, 15>, array<char, 32>>, ArrayHasher> table;

void fdSetRefresh()
{
    FD_ZERO(&readfds);
    for(auto& fd : fds)
    {
        FD_SET(fd, &readfds);
    }
}

int maxFd()
{
    int retVal = -1;
    for(auto& fd : fds)
    {
        if(fd > retVal)
        {
            retVal = fd;
        }
    }
    return retVal;
}

int main()
{
    int ret;
    if(filesystem::is_socket(SOCKET_NAME))
    {
        ret = unlink(SOCKET_NAME);
        if(ret == -1)
        {
            cerr<<"unlink"<<endl;;
            return EXIT_FAILURE;
        }
    }
    else if(filesystem::exists(SOCKET_NAME))
    {
        cerr<<"Existing file with SOCKET_NAME, won't unlink"<<endl;
        return EXIT_FAILURE;
    }
    int connSocket = socket(AF_UNIX, SOCK_STREAM, 0);
    if(connSocket == -1)
    {
        cerr<<"socket"<<endl;
        return EXIT_FAILURE;
    }
    fds.insert(connSocket);
    sockaddr_un socketName;
    socketName.sun_family = AF_UNIX;
    strcpy(socketName.sun_path, SOCKET_NAME);
    ret = bind(connSocket, (const sockaddr *) &socketName, sizeof(socketName));
    if(ret == -1)
    {
        cerr<<"bind"<<endl;
        return EXIT_FAILURE;
    }
    listen(connSocket, 30);
    if(ret == -1)
    {
        cerr<<"listen"<<endl;
        return EXIT_FAILURE;
    }
    while(1)
    {
        fdSetRefresh();
        ret = select(maxFd() + 1, &readfds, NULL, NULL, NULL);
        if(ret == -1)
        {
            cerr<<"select"<<endl;
            return EXIT_FAILURE;
        }
        if(FD_ISSET(connSocket,&readfds))
        {
            int dataSocket = accept(connSocket, NULL, NULL);
            if(dataSocket == -1)
            {
                cerr<<"accept"<<endl;
                return EXIT_FAILURE;
            }
            cout<<"Connection request from client accepted"<<endl;
            fds.insert(dataSocket);
            cout<<"Dumping table to client..."<<endl;
            sync_msg_t outgoingMsg;
            outgoingMsg.op_code = OPCODE::DUMP_START;
            write(dataSocket, &outgoingMsg, sizeof(sync_msg_t));
            outgoingMsg.op_code = OPCODE::CREATE;
            for(auto& row : table)
            {
                memcpy(&outgoingMsg.msg_body.destination,&row.first,sizeof(outgoingMsg.msg_body.destination));
                memcpy(&outgoingMsg.msg_body.mask,&row.first[16],sizeof(outgoingMsg.msg_body.mask));
                memcpy(&outgoingMsg.msg_body.gateway_ip,&row.second.first,sizeof(outgoingMsg.msg_body.gateway_ip));
                memcpy(&outgoingMsg.msg_body.output_interface,&row.second.second,sizeof(outgoingMsg.msg_body.output_interface));
                write(dataSocket, &outgoingMsg, sizeof(sync_msg_t));
            }
            outgoingMsg.op_code = OPCODE::DUMP_END;
            write(dataSocket, &outgoingMsg, sizeof(sync_msg_t));
            cout<<"Dumping finished"<<endl;
        }
        else
        {
            cout<<"Receiving instruction"<<endl;
            for(auto& fd : fds)
            {
                if(FD_ISSET(fd, &readfds))
                {
                    sync_msg_t incomingMsg;
                    ret = read(fd, &incomingMsg, sizeof(sync_msg_t));
                    if(ret == -1)
                    {
                        cerr<<"read"<<endl;
                        return EXIT_FAILURE;
                    }
                    cout<<"Received!"<<endl;
                    array<char, 17> incomingDest;
                    memcpy(&incomingDest,&incomingMsg.msg_body.destination,sizeof(incomingMsg.msg_body.destination));
                    incomingDest[15] = '/';
                    incomingDest.back() = incomingMsg.msg_body.mask;
                    if(incomingMsg.op_code == OPCODE::CREATE)
                    {
                        pair<array<char, 15>, array<char, 32>> inData;
                        inData.first = incomingMsg.msg_body.gateway_ip;
                        inData.second = incomingMsg.msg_body.output_interface;
                        if(table.count(incomingDest) > 0)
                        {
                            sync_msg_t errorMsg;
                            errorMsg.op_code = OPCODE::CREATE_ERROR;
                            cout<<"Error: Row could not be added"<<endl;
                            write(fd, &errorMsg, sizeof(sync_msg_t));
                        }
                        else
                        {
                            table[incomingDest] = inData;
                            cout<<"New row added"<<endl;
                            for(auto& stakeHolder : fds)
                            {
                                write(stakeHolder, &incomingMsg, sizeof(sync_msg_t));
                            }
                        }
                    }
                    else if(incomingMsg.op_code == OPCODE::UPDATE)
                    {
                        pair<array<char, 15>, array<char, 32>> inData;
                        inData.first = incomingMsg.msg_body.gateway_ip;
                        inData.second = incomingMsg.msg_body.output_interface;
                        if(table.count(incomingDest) == 0)
                        {
                            sync_msg_t errorMsg;
                            errorMsg.op_code = OPCODE::UPDATE_ERROR;
                            cout<<"Error: Update not performed"<<endl;
                            write(fd, &errorMsg, sizeof(sync_msg_t));
                        }
                        else
                        {
                            table[incomingDest] = inData;
                            cout<<"Row updated"<<endl;
                            for(auto& stakeHolder : fds)
                            {
                                write(stakeHolder, &incomingMsg, sizeof(sync_msg_t));
                            }
                        }
                    }
                    else if(incomingMsg.op_code == OPCODE::DELETE)
                    {
                        if(table.count(incomingDest) == 0)
                        {
                            sync_msg_t errorMsg;
                            errorMsg.op_code = OPCODE::DELETE_ERROR;
                            cout<<"Error: Deletion not performed"<<endl;
                            write(fd, &errorMsg, sizeof(sync_msg_t));
                        }
                        else
                        {
                            table.erase(incomingDest);
                            cout<<"Row deleted"<<endl;
                            for(auto& stakeHolder : fds)
                            {
                                write(stakeHolder, &incomingMsg, sizeof(sync_msg_t));
                            }
                        } 
                    }
                    break;
                }
            }
        }
    }
    ret = close(connSocket);
    if(ret == -1)
    {
        cerr<<"close"<<endl;
        return EXIT_FAILURE;
    }
    cout<<"Server terminated"<<endl;
    ret = unlink(SOCKET_NAME);
    if(ret == -1)
    {
        cerr<<"unlink"<<endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}