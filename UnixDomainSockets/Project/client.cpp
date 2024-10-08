#include <iostream>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/select.h>
#include <unistd.h>
#include <unordered_set>
#include <unordered_map>
#include <array>
#include "project_socket.h"

using namespace std;

unordered_map<array<char, 17>, pair<array<char, 15>, array<char, 32>>, ArrayHasher> table;

int main()
{
    int dataSocket = socket(AF_UNIX, SOCK_STREAM, 0);
    if(dataSocket == -1)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    sockaddr_un socketAddr;
    socketAddr.sun_family = AF_UNIX;
    strcpy(socketAddr.sun_path, SOCKET_NAME);
    int ret = connect(dataSocket, (const sockaddr *) &socketAddr, sizeof(socketAddr));
    if(ret == -1)
    {
        perror("connect");
        exit(EXIT_FAILURE);
    }
    while(1)
    {
        sync_msg_t incomingMsg;
        ret = read(dataSocket, &incomingMsg, sizeof(sync_msg_t));
        if(ret == -1)
        {
            perror("read");
            exit(EXIT_FAILURE);
        }
        array<char, 17> incomingDest;
        memcpy(&incomingDest,&incomingMsg.msg_body.destination,sizeof(incomingMsg.msg_body.destination));
        incomingDest[15] = '/';
        incomingDest.back() = incomingMsg.msg_body.mask;
        if(incomingMsg.op_code == OPCODE::CREATE || incomingMsg.op_code == OPCODE::UPDATE)
        {
            pair<array<char, 15>, array<char, 32>> inData;
            inData.first = incomingMsg.msg_body.gateway_ip;
            inData.second = incomingMsg.msg_body.output_interface;
            table[incomingDest] = inData;
        }
        else if(incomingMsg.op_code == OPCODE::DELETE)
        {
            table.erase(incomingDest);
        }
        else if(incomingMsg.op_code == OPCODE::DUMP_START)
        {
            while(1)
            {
                ret = read(dataSocket, &incomingMsg, sizeof(sync_msg_t));
                if(ret == -1)
                {
                    cerr<<"read"<<endl;
                    return EXIT_FAILURE;
                }
                if(incomingMsg.op_code != OPCODE::DUMP_END)
                {
                    memcpy(&incomingDest,&incomingMsg.msg_body.destination,sizeof(incomingMsg.msg_body.destination));
                    incomingDest[15] = '/';
                    incomingDest.back() = incomingMsg.msg_body.mask;
                    
                    pair<array<char, 15>, array<char, 32>> inData;
                    inData.first = incomingMsg.msg_body.gateway_ip;
                    inData.second = incomingMsg.msg_body.output_interface;
                    table[incomingDest] = inData;
                }
                else
                {
                    break;
                }
            }
        }
        else
        {
            break;
        }
        system("clear");
        cout<<"_________________________________________________________________________________"<<endl;
        cout<<"|  Destination Subnet  |  Gateway IP       |  Output Interface                  |"<<endl;
        for(auto& row : table)
        {
            cout<<"_________________________________________________________________________________"<<endl;
            cout<<"|  ";
            for(int charIndex = 0; charIndex < row.first.size() - 1; charIndex++)
            {
                cout<<row.first[charIndex];
            }
            cout<<static_cast<int>(row.first.back());
            if(row.first.back() <= 9)
            {
                cout<<" ";
            }
            cout<<"  |  ";
            for(auto& character : row.second.first)
            {
                cout<<character;
            }
            cout<<"  |  ";
            for(auto& character : row.second.second)
            {
                cout<<character;
            }
            cout<<"  |"<<endl;
        }
        cout<<"_________________________________________________________________________________"<<endl<<endl;
    }
    ret = close(dataSocket);
    if(ret == -1)
    {
        perror("close");
        exit(EXIT_FAILURE);
    }
    cout<<"Connection closed"<<endl;
    exit(EXIT_SUCCESS);
}