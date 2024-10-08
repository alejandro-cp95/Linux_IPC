#include <iostream>
#include <string>
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
        cerr<<"socket"<<endl;
        return EXIT_FAILURE;
    }
    sockaddr_un socketAddr;
    socketAddr.sun_family = AF_UNIX;
    strcpy(socketAddr.sun_path, SOCKET_NAME);
    int ret = connect(dataSocket, (const sockaddr *) &socketAddr, sizeof(socketAddr));
    if(ret == -1)
    {
        cerr<<"connect"<<endl;
        return EXIT_FAILURE;
    }
    while(1)
    {
        system("clear");
        sync_msg_t incomingMsg;
        ret = read(dataSocket, &incomingMsg, sizeof(sync_msg_t));
        if(ret == -1)
        {
            cerr<<"read"<<endl;
            return EXIT_FAILURE;
        }
        array<char, 17> incomingDest;
        memcpy(&incomingDest,&incomingMsg.msg_body.destination,sizeof(incomingMsg.msg_body.destination));
        incomingDest[15] = '/';
        incomingDest.back() = incomingMsg.msg_body.mask;
        if(incomingMsg.op_code == OPCODE::CREATE)
        {
            pair<array<char, 15>, array<char, 32>> inData;
            inData.first = incomingMsg.msg_body.gateway_ip;
            inData.second = incomingMsg.msg_body.output_interface;
            table[incomingDest] = inData;
        }
        else if(incomingMsg.op_code == OPCODE::CREATE_ERROR)
        {
            cout<<"Error: Row could not be added"<<endl;
        }
        else if(incomingMsg.op_code == OPCODE::UPDATE)
        {
            pair<array<char, 15>, array<char, 32>> inData;
            inData.first = incomingMsg.msg_body.gateway_ip;
            inData.second = incomingMsg.msg_body.output_interface;
            table[incomingDest] = inData;
        }
        else if(incomingMsg.op_code == OPCODE::UPDATE_ERROR)
        {
            cout<<"Error: Update not performed"<<endl;
        }
        else if(incomingMsg.op_code == OPCODE::DELETE)
        {
            table.erase(incomingDest);
        }
        else if(incomingMsg.op_code == OPCODE::DELETE_ERROR)
        {
            cout<<"Error: Deletion not performed"<<endl;
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
        cout<<"Select an option:"<<endl<<"1) Add row"<<endl<<"2) Modify row"<<endl<<"3) Delete row"<<endl;
        int option;
        cin>>option;
        if(option == 1)
        {
            sync_msg_t outgoingMsg;
            outgoingMsg.op_code = OPCODE::CREATE;
            string input;
            cout<<"Enter destination IP address:"<<endl;
            cin>>input;
            for(int charIndex = 0; charIndex < 15; charIndex++)
            {
                if(charIndex < 15 - input.size())
                {
                    outgoingMsg.msg_body.destination[charIndex] = ' ';
                }
                else
                {
                    outgoingMsg.msg_body.destination[charIndex] = input[charIndex - (15 - input.size())];
                }
            }
            cout<<"Enter destination mask:"<<endl;
            cin>>input;
            outgoingMsg.msg_body.mask = static_cast<char>(stoi(input));
            cout<<"Enter gateway IP:"<<endl;
            cin>>input;
            for(int charIndex = 0; charIndex < 15; charIndex++)
            {
                if(charIndex < 15 - input.size())
                {
                    outgoingMsg.msg_body.gateway_ip[charIndex] = ' ';
                }
                else
                {
                    outgoingMsg.msg_body.gateway_ip[charIndex] = input[charIndex - (15 - input.size())];
                }
            }
            cout<<"Enter output interface:"<<endl;
            cin>>input;
            for(int charIndex = 0; charIndex < 32; charIndex++)
            {
                if(charIndex >= 32 - (32 - input.size()))
                {
                    outgoingMsg.msg_body.output_interface[charIndex] = ' ';
                }
                else
                {
                    outgoingMsg.msg_body.output_interface[charIndex] = input[charIndex];
                }
            }
            write(dataSocket, &outgoingMsg, sizeof(sync_msg_t));
        }
        else if(option == 2)
        {
            sync_msg_t outgoingMsg;
            outgoingMsg.op_code = OPCODE::UPDATE;
            string input;
            cout<<"Enter destination IP address:"<<endl;
            cin>>input;
            for(int charIndex = 0; charIndex < 15; charIndex++)
            {
                if(charIndex < 15 - input.size())
                {
                    outgoingMsg.msg_body.destination[charIndex] = ' ';
                }
                else
                {
                    outgoingMsg.msg_body.destination[charIndex] = input[charIndex - (15 - input.size())];
                }
            }
            cout<<"Enter destination mask:"<<endl;
            cin>>input;
            outgoingMsg.msg_body.mask = static_cast<char>(stoi(input));
            cout<<"Enter gateway IP:"<<endl;
            cin>>input;
            for(int charIndex = 0; charIndex < 15; charIndex++)
            {
                if(charIndex < 15 - input.size())
                {
                    outgoingMsg.msg_body.gateway_ip[charIndex] = ' ';
                }
                else
                {
                    outgoingMsg.msg_body.gateway_ip[charIndex] = input[charIndex - (15 - input.size())];
                }
            }
            cout<<"Enter output interface:"<<endl;
            cin>>input;
            for(int charIndex = 0; charIndex < 32; charIndex++)
            {
                if(charIndex >= 32 - (32 - input.size()))
                {
                    outgoingMsg.msg_body.output_interface[charIndex] = ' ';
                }
                else
                {
                    outgoingMsg.msg_body.output_interface[charIndex] = input[charIndex];
                }
            }
            write(dataSocket, &outgoingMsg, sizeof(sync_msg_t));
        }
        else
        {
            sync_msg_t outgoingMsg;
            outgoingMsg.op_code = OPCODE::DELETE;
            string input;
            cout<<"Enter destination IP address:"<<endl;
            cin>>input;
            for(int charIndex = 0; charIndex < 15; charIndex++)
            {
                if(charIndex < 15 - input.size())
                {
                    outgoingMsg.msg_body.destination[charIndex] = ' ';
                }
                else
                {
                    outgoingMsg.msg_body.destination[charIndex] = input[charIndex - (15 - input.size())];
                }
            }
            cout<<"Enter destination mask:"<<endl;
            cin>>input;
            outgoingMsg.msg_body.mask = static_cast<char>(stoi(input));
            write(dataSocket, &outgoingMsg, sizeof(sync_msg_t));
        }
    }
    ret = close(dataSocket);
    if(ret == -1)
    {
        cerr<<"close"<<endl;
        return EXIT_FAILURE;
    }
    cout<<"Connection closed"<<endl;
    return EXIT_SUCCESS;
}