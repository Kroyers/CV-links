//
// Created by Qingyun on 29.11.2019.
//

#include "Connection.h"
#include "Header.h"
#include "Transmission.h"
#include <fcntl.h>
#include <iostream>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <thread>

namespace stp{
    extern Connection connections[MAX_NUMBER_OF_CONNECTIONS];

    char empty_array[0];

    Option emptyOption[0];

//Initialize send and receive window (NOT the buffer!)
    void initWnd(int connectionID, uint32_t sendWndInput, uint32_t rcvWndInput){
        connections[connectionID].sendWnd = sendWndInput;
        connections[connectionID].rcvWnd = rcvWndInput;
    }

//Gets the addrinfo of local host on the given port
    bool serverGetLocalAddrinfo(int connectionID, const char* port, uint8_t protocol){
        int status;
        struct addrinfo hints;

        memset(&hints, 0, sizeof(hints));
        if (protocol == 4){
            hints.ai_family = AF_INET;
            connections[connectionID].protocol = 4;
        }
        else{
            hints.ai_family = AF_INET6;
        }
        hints.ai_protocol = IPPROTO_UDP;
        hints.ai_socktype = SOCK_DGRAM;
        hints.ai_flags = AI_PASSIVE;

        status = getaddrinfo(NULL, port, &hints, &connections[connectionID].locAddrInfo);
        return status == 0;
    }

//Gets the addrinfo of remote host with given IP and Port
    bool getRemoteAddrinfo(int connectionID, char* ip, char* port){
        struct addrinfo hints;
        int success;

        memset(&hints, 0, sizeof(hints));
        hints.ai_socktype = SOCK_DGRAM;
        hints.ai_protocol = IPPROTO_UDP;

        success = getaddrinfo(ip, port, &hints, &connections[connectionID].remoteAddrInfo);
        if(success != 0){
            freeaddrinfo(connections[connectionID].remoteAddrInfo);
            return false;
        }
        else{
            return true;
        }
    }

//Try to create a socket
    bool clientCreateSocket(int connectionID, uint8_t protocol){
        int sockID;

        if (protocol == 4){
            sockID = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
            connections[connectionID].protocol = 4;
        }
        else{
            sockID = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
        }

        if (sockID < 0){
            return false;
        }
        else{
            connections[connectionID].sockfd = sockID;
        }

        struct timeval tv = {0,100000};
        if(setsockopt(connections[connectionID].sockfd, SOL_SOCKET, SO_RCVTIMEO,(struct timeval *)&tv, sizeof(struct timeval)) < 0){
            return false;
        }
        if(setsockopt(connections[connectionID].sockfd, SOL_SOCKET, SO_SNDTIMEO,(struct timeval *)&tv, sizeof(struct timeval)) < 0){
            return false;
        }

        return true;
    }

    bool serverSetTimeout(int connectionID){
        struct timeval tv = {0,100000};
        if(setsockopt(connections[connectionID].sockfd, SOL_SOCKET, SO_RCVTIMEO,(struct timeval *)&tv, sizeof(struct timeval)) < 0){
            return false;
        }
        if(setsockopt(connections[connectionID].sockfd, SOL_SOCKET, SO_SNDTIMEO,(struct timeval *)&tv, sizeof(struct timeval)) < 0){
            return false;
        }
    }

    bool serverCreateSocket(int connectionID, int protocol){
        int sockID;

        sockID = socket(connections[connectionID].locAddrInfo->ai_family, SOCK_DGRAM, IPPROTO_UDP);
        if (sockID < 0){
            return false;
        }

        connections[connectionID].sockfd = sockID;

        int no = 0;
        /*if(protocol != 4){
            if(setsockopt(connections[connectionID].sockfd, IPPROTO_IPV6, IPV6_V6ONLY, (void *)&no, sizeof(int)) < 0){
                return false;
            }
        }*/


        return true;
    }

//Sends a contol information packet with no payload
    bool sendCtrInfoPacket(int connectionID, uint8_t flags, Option *options, uint8_t optLenInput){
        bool success;
        success = stpSend(connectionID, empty_array, 0, flags, options, optLenInput, connections[connectionID].seqNext);
        return success;
    }

//Look up addrInfo for local and remote host, create and bind socket.
    bool clientSocketSetUp(int connectionID, char* ip, char* port, int protocol){
        bool success;

        success = getRemoteAddrinfo(connectionID, ip, port);

        if(connections[connectionID].remoteAddrInfo->ai_family == AF_INET6){
        }

        if(connections[connectionID].remoteAddrInfo->ai_family == AF_INET){
        }

        if(!success){
            freeaddrinfo(connections[connectionID].remoteAddrInfo);
            return false;
        }
        else{
        }

        //Set global variable
        connections[connectionID].dst = connections[connectionID].remoteAddrInfo->ai_addr;
        connections[connectionID].dstLen = connections[connectionID].remoteAddrInfo->ai_addrlen;
        connections[connectionID].dstLenPtr = &connections[connectionID].dstLen;

        //Create socket
        success = clientCreateSocket(connectionID, protocol);
        if (!success){
            freeaddrinfo(connections[connectionID].remoteAddrInfo);
            return false;
        }
        else{
        }

        return true;
    }

    //Look up addrInfo for local and remote host, create and bind socket.
    bool serverLocalSocketSetUp(int connectionID, char* localPort,uint8_t protocol){
        bool success;
        int status;

        success = serverGetLocalAddrinfo(connectionID, localPort, protocol);
        if(!success){
            freeaddrinfo(connections[connectionID].locAddrInfo);
            std::cerr << "Local Addrinfo failure" << std::endl;
            return false;
        }

        //Create socket
        success = serverCreateSocket(connectionID, protocol);
        if (!success){
            freeaddrinfo(connections[connectionID].locAddrInfo);
            freeaddrinfo(connections[connectionID].remoteAddrInfo);
            std::cerr << "Socket failure" << std::endl;
            return false;
        }
        else{
        }

        //Bind to socket
        status = bind(connections[connectionID].sockfd, connections[connectionID].locAddrInfo->ai_addr, connections[connectionID].locAddrInfo->ai_addrlen);
        if(status < 0){
            freeaddrinfo(connections[connectionID].locAddrInfo);
            freeaddrinfo(connections[connectionID].remoteAddrInfo);
            close(connections[connectionID].sockfd);
            std::cerr << "Bind failure" << std::endl;
            return false;
        }
        else{
        }
        return true;
    }

//Set up local enviroment for STP including Buffer, window, socket, addrinfo ...
    bool clientLocalSetUp(int connectionID, char* ip, char* port, uint32_t sendWndInput, uint32_t rcvWndInput,uint8_t protocol){
        bool success;

        connections[connectionID].workingBuffer = new char[65535];

        //Sets our window variables
        initWnd(connectionID, sendWndInput, rcvWndInput);

        //Look up addrInfo for local and remote host, create and bind socket.
        success = clientSocketSetUp(connectionID, ip, port, protocol);
        if (!success){
            //freeaddrinfo(connections[connectionID].locAddrInfo);
            freeaddrinfo(connections[connectionID].remoteAddrInfo);
            close(connections[connectionID].sockfd);
            return false;
        }

        //Initializes our buffers
        if(initBuffer(connectionID, sendWndInput, rcvWndInput) == true){
        }
        else{
            //freeaddrinfo(connections[connectionID].locAddrInfo);
            freeaddrinfo(connections[connectionID].remoteAddrInfo);
            close(connections[connectionID].sockfd);
            return false;
        }

        //Initializes ISN
        initSeqNr(connectionID);
        return true;
    }

    //Set up local enviroment for STP including Buffer, window, socket, addrinfo ...
    bool serverLocalSetUp(int connectionID, char* localPort, uint32_t sendWndInput, uint32_t rcvWndInput, uint8_t protocol){
        bool success;

        connections[connectionID].workingBuffer = new char[65535];

        //Sets our window variables
        initWnd(connectionID, sendWndInput, rcvWndInput);

        //Look up addrInfo for local host, create and bind socket.
        success = serverLocalSocketSetUp(connectionID, localPort, protocol);
        if (!success){
            freeaddrinfo(connections[connectionID].locAddrInfo);
            freeaddrinfo(connections[connectionID].remoteAddrInfo);
            close(connections[connectionID].sockfd);
            return false;
        }

        //Initializes our buffers
        if(initBuffer(connectionID, sendWndInput, rcvWndInput) == true){
        }
        else{
            freeaddrinfo(connections[connectionID].locAddrInfo);
            freeaddrinfo(connections[connectionID].remoteAddrInfo);
            close(connections[connectionID].sockfd);
            return false;
        }

        //Initializes ISN
        initSeqNr(connectionID);
        return true;
    }

    void freeConnectionSlot(int connectionID){
        connections[connectionID].isFree = true;
    }

    int lookForFreeConnectionSlot(){
        for (int i = 0; i < MAX_NUMBER_OF_CONNECTIONS; i++) {
            if (connections[i].isFree){
                connections[i].isFree = false;
                return i;
            }
        }
        return -1;
    }

    /*void sendThread(int connectionID){
        std::cerr << "In send thread------------------------------------------------------------" << std::endl;
        Option* nackOpt = new Option;
        uint8_t optLen = 0;
        if (connections[connectionID].NACKUsed && (connections[connectionID].nackPhase == 1)){
            nackOpt = nack_list(connectionID);
            optLen++;
        }
        while(connections[connectionID].connectionPhase == CONTEXT_DATA_TRANS){
            //if (connections[connectionID].seqUnack == connections[connectionID].seqNext){
                stpSend(connectionID, connections[connectionID].inputBuffer, connections[connectionID].inputBufferUsed,0,
                        nackOpt, optLen, connections[connectionID].seqNext);
            //}
            connections[connectionID].inputBufferUsed = 0;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        delete(nackOpt);
    }*/

    void receiveThread(int connectionID){
        while(connections[connectionID].connectionPhase == CONTEXT_DATA_TRANS && !connections[connectionID].connectionTerminated){
            //std::cerr << "In recv thread------------------------------------------------------------" << std::endl;
            stpReceive(connectionID);
            std::this_thread::sleep_for(std::chrono::milliseconds(2000));
        }
    }

    void dataTransmission(int connectionID){
        //std::thread threadSend(sendThread, connectionID);
        //threadSend.detach();
        std::thread threadRecv(receiveThread, connectionID);
        threadRecv.detach();
    }

//This method should allow establishing a connection between two STP instances.
//Return negative value for each different failure, returns sockfd on success
//int connect(char* ip, char* port){
//int stp_connect(struct sockaddr *dstInput, uint32_t sendWndInput, uint32_t rcvWndInput){
    int connect(char* ip, char* port, uint8_t protocol, uint32_t sendWndInput, uint32_t rcvWndInput, Option options[],
                uint8_t optLenInput){
        int connectionID = lookForFreeConnectionSlot();
        if(connectionID < 0){
            return -1;
        }
        connections[connectionID].isFree = false;
        connections[connectionID].connectionTerminated = false;
        if (!clientLocalSetUp(connectionID, ip, port, sendWndInput, rcvWndInput, protocol)){
            return -1;
        }


        for (int i = 0; i < optLenInput; i++) {
            if (options[i].type == OPTION_CONGESTION_CONTROL){
                connections[connectionID].congestionControlUsed = true;
            }
        }


        //Set up global variables for options
        connections[connectionID].optionsUsed = options;
        connections[connectionID].optLenUsed = optLenInput;
        connections[connectionID].remoteIP = ip;
        connections[connectionID].remotePort = port;

        //Starts global timer
        startTimers(connectionID);


        //Sends SYN packet
        if(!sendCtrInfoPacket(connectionID, SYN_FLAG, connections[connectionID].optionsUsed, connections[connectionID].optLenUsed)){
            freeaddrinfo(connections[connectionID].remoteAddrInfo);
            close(connections[connectionID].sockfd);
            freeConnectionSlot(connectionID);
            return -1;
        }
        else{
        }
        //std::cerr << "Sent SYN" << std::endl;

        //Change connection mode
        connections[connectionID].connectionPhase = CONTEXT_SYN_SENT;

        //Receives SYN,ACK
        if(stpReceive(connectionID) < 0){
            freeaddrinfo(connections[connectionID].remoteAddrInfo);
            close(connections[connectionID].sockfd);
            freeConnectionSlot(connectionID);
            return -1;
        }
        else{
        }
        //std::cerr << "Recved SYN ACK" << std::endl;


        //Change connection mode
        connections[connectionID].connectionPhase = CONTEXT_SYN_RECEIVE;

        //Sends ACK packet
        if(!sendCtrInfoPacket(connectionID, ACK_FLAG, connections[connectionID].optionsUsed, connections[connectionID].optLenUsed)){
            freeaddrinfo(connections[connectionID].remoteAddrInfo);
            close(connections[connectionID].sockfd);
            freeConnectionSlot(connectionID);
            return -1;
        }
        else{
            freeaddrinfo(connections[connectionID].remoteAddrInfo);
        }
        //std::cerr << "Sent ACK" << std::endl;

        connections[connectionID].connectionPhase = CONTEXT_DATA_TRANS;

        dataTransmission(connectionID);

        return connectionID;
    }

//Simple listening function that listens on one socket, when it receives a correct STP SYN, it calls accept
    int listen(char* localPort, uint8_t protocol, uint32_t sendWndInput, uint32_t rcvWndInput, Option options[], uint8_t optLenInput){
        int connectionID = lookForFreeConnectionSlot();
        if (connectionID < 0){
            return -1;
        }
        connections[connectionID].isFree = false;
        connections[connectionID].connectionTerminated = false;
        if (!serverLocalSetUp(connectionID, localPort, sendWndInput, rcvWndInput, protocol)){
            return -1;
        }

        for (int i = 0; i < optLenInput; i++) {
            if (options[i].type == OPTION_CONGESTION_CONTROL){
                connections[connectionID].congestionControlUsed = true;
            }
        }

        //set global variable for options
        connections[connectionID].optionsUsed = options;
        connections[connectionID].optLenUsed = optLenInput;
        connections[connectionID].localPort = localPort;

        //Perform Handshake
        connections[connectionID].connectionPhase = CONTEXT_SYN_EXPECTED;
        bool synRecved = false;

        while(!synRecved){
            //Receives SYN
            if(stpReceive(connectionID) < 0){
            }
            else{
                synRecved = true;
            }
        }

        return connectionID;
    }

//This method sends a FIN or FIN ACK message to signalise that it has nothing more to send
//Return negative value on failure, 0 on success
    int stpClose(int connectionID){
        if(connections[connectionID].finRcved == false){
            //We send FIN packet
            if(!sendCtrInfoPacket(connectionID, FIN_FLAG, emptyOption, 0)){
                close(connections[connectionID].sockfd);
                return -1;
            }

            //We change connection phase
            connections[connectionID].connectionPhase = CONTEXT_FIN_SENT;

            while (!connections[connectionID].connectionTerminated){
                stpReceive(connectionID);
            }
        }
        else{
            //We send FIN, ACK packet
            if(!sendCtrInfoPacket(connectionID, FIN_ACK_FLAG, emptyOption, 0)){
                return -1;
            }

            //We change connection phase
            connections[connectionID].connectionPhase = CONTEXT_FIN_ACK_SENT;

            while (!connections[connectionID].connectionTerminated){
                stpReceive(connectionID);
            }
        }
        return 0;
    }

//Sends only finack, depends on the mode, either wait for a FIN ACK or terminate connection.
    int sendFinAck(int connectionID, int mode){
        //We send FIN, ACK packet
        if(!sendCtrInfoPacket(connectionID, FIN_ACK_FLAG, emptyOption, 0)){
            close(connections[connectionID].sockfd);
            return -1;
        }

        if (mode == FIN_ACK_TERMINATE_MODE){
            //std::cerr << "Final FINACK sent" << std::endl;
            terminateConnection(connectionID);
            return 0;
        }
        else if(mode == FIN_ACK_WAIT_MODE){
            //Change connection mode
            connections[connectionID].connectionPhase = CONTEXT_FIN_ACK_SENT;
            //std::cerr << "First FINACK sent" << std::endl;
            int len = -1;
            while (len < 0) {
                len = stpReceive(connectionID);
            }
            //std::cerr << "First FINACK recved" << std::endl;
            return 0;
        }
        return 0;
    }

    void resetConnectionInfo(int connectionID){
        connections[connectionID].isFree = true;
        //connections[connectionID].locAddrInfo = nullptr;
        //connections[connectionID].remoteAddrInfo = nullptr;
        connections[connectionID].seqUnack = 0;
        connections[connectionID].seqNext = 0;
        connections[connectionID].seqExpected = 0;
        connections[connectionID].seqToack = 0;
        connections[connectionID].rcvWnd = 3000;
        connections[connectionID].sendWnd = 3000;
        connections[connectionID].seqLastRead = 0;
        //connections[connectionID].dst = nullptr;
        //connections[connectionID].sockfd = -1;
        connections[connectionID].dstLen = sizeof(sockaddr_storage);
        connections[connectionID].dstLenPtr = &connections[connectionID].dstLen;
        connections[connectionID].connectionPhase = -1;
        connections[connectionID].finRcved = false;
        connections[connectionID].connectionTerminated = false;
        connections[connectionID].versionUsed = 3;
        connections[connectionID].optLenUsed = 0;
        connections[connectionID].congestionControlUsed = false;
        connections[connectionID].NACKUsed = false;
        connections[connectionID].dataCompressionUsed = false;
        connections[connectionID].dataCompressionAlgo = 0;
        connections[connectionID].sendRingBuffer = new BufferPacket*[MAX_RINGBUFFER_SLOT];
        connections[connectionID].sendBufferUsed = 0;
        connections[connectionID].sendBufferHead = 0;
        connections[connectionID].sendBufferTail = 0;
        connections[connectionID].sendBufferMaxSize = MAX_BUFFER_SIZE;
        connections[connectionID].rcvRingBuffer = new BufferPacket*[MAX_RINGBUFFER_SLOT];
        connections[connectionID].receiveBufferUsed = 0;
        connections[connectionID].receiveBufferHead = 0;
        connections[connectionID].receiveBufferTail = 0;
        connections[connectionID].receiveBufferMaxSize = MAX_BUFFER_SIZE;
        connections[connectionID].cWnd = MIN_CONGESTION_WINDOW;
        connections[connectionID].cThreshold = STARTING_THRESHOLD;
        connections[connectionID].cCounter = 0;
        connections[connectionID].cTimer = NOT_EXISTING_TIMER;
        connections[connectionID].globalTimer = NOT_EXISTING_TIMER;
        connections[connectionID].heartbeatTimer = NOT_EXISTING_TIMER;
        connections[connectionID].retransTimer = NOT_EXISTING_TIMER;
        connections[connectionID].globalTime = GLOBAL_TIMEOUT_MILLISEC;
        connections[connectionID].heartbeatTime = HEARTBEAT_TIMEOUT_MILLISEC;
        connections[connectionID].retransTime = 2000;
        connections[connectionID].resetGlobalTimer = false;
        connections[connectionID].resetHeartbeatTimer = false;
        connections[connectionID].resetRetransTimer = false;
        connections[connectionID].nCounter = 0;
        connections[connectionID].localPort = nullptr;
        connections[connectionID].remotePort = nullptr;
        connections[connectionID].remoteIP = nullptr;
        connections[connectionID].nackPhase = 0;
        connections[connectionID].inputBufferUsed = 0;
        connections[connectionID].protocol = 6;
    }

//Return negative value on failure, 0 on success
    int terminateConnection(int connectionID){
        if(connections[connectionID].connectionTerminated){
            std::cerr << "Connection is already terminated" << std::endl;
            return -1;
        }
        clearBuffer(connectionID);
        //std::cerr << "Clear Buffer finished" << std::endl;
        clear_nack(connectionID);
        //std::cerr << "Clear NACK finished" << std::endl;
        close(connections[connectionID].sockfd);
        //std::cerr << "Close socket finished" << std::endl;
        delete[] connections[connectionID].workingBuffer;
        //std::cerr << "Delete working buffer finished" << std::endl;
        resetConnectionInfo(connectionID);
        connections[connectionID].connectionTerminated = true;
        std::cerr << "Connection Terminated for ID: " << connectionID << std::endl;
        return 0;
    }

//Function to accept a new connection
    int accept(int connectionID){
        //Starts global timer
        serverSetTimeout(connectionID);
        startTimers(connectionID);

        if(connections[connectionID].dataCompressionUsed&&connections[connectionID].NACKUsed){
            Option nackOption;
            nackOption.type = OPTION_NACK;
            nackOption.info = 0;
            nackOption.length = 0;

            Option dcOption;
            dcOption.type = OPTION_DATA_COMPRESSION;
            dcOption.info = connections[connectionID].dataCompressionAlgo << 16;
            dcOption.length = 8;

            Option options[2] = {nackOption, dcOption};
            connections[connectionID].optionsUsed = options;
            connections[connectionID].optLenUsed = 2;
        }
        else if(connections[connectionID].NACKUsed){
            Option nackOption;
            nackOption.type = OPTION_NACK;
            nackOption.info = 0;
            nackOption.length = 0;

            Option options[1] = {nackOption};
            connections[connectionID].optionsUsed = options;
            connections[connectionID].optLenUsed = 1;
        }
        else if(connections[connectionID].dataCompressionUsed){
            Option dcOption;
            dcOption.type = OPTION_DATA_COMPRESSION;
            dcOption.info = connections[connectionID].dataCompressionAlgo << 16;
            dcOption.length = 8;

            Option options[1] = {dcOption};
            connections[connectionID].optionsUsed = options;
            connections[connectionID].optLenUsed = 1;
        }
        else{
            Option options[1] = {};
            connections[connectionID].optionsUsed = options;
            connections[connectionID].optLenUsed = 0;
        }

        if(!sendCtrInfoPacket(connectionID, SYN_ACK_FLAG, connections[connectionID].optionsUsed, connections[connectionID].optLenUsed)){
            freeaddrinfo(connections[connectionID].remoteAddrInfo);
            freeaddrinfo(connections[connectionID].locAddrInfo);
            close(connections[connectionID].sockfd);
            freeConnectionSlot(connectionID);
            return -1;
        }
        else{
        }

        //Change connection mode
        connections[connectionID].connectionPhase = CONTEXT_SYN_ACK_SENT;

        //Receives ACK
        if(stpReceive(connectionID) < 0){
            freeaddrinfo(connections[connectionID].remoteAddrInfo);
            freeaddrinfo(connections[connectionID].locAddrInfo);
            close(connections[connectionID].sockfd);
            freeConnectionSlot(connectionID);
            return -1;
        }
        else{
        }

        dataTransmission(connectionID);

        //stp_transmission_phase();
        connections[connectionID].connectionPhase = CONTEXT_DATA_TRANS;
        freeaddrinfo(connections[connectionID].remoteAddrInfo);
        freeaddrinfo(connections[connectionID].locAddrInfo);
    }

//Function to reset a connection
    void reset(int connectionID){
        sendCtrInfoPacket(connectionID, RST_FLAG, connections[connectionID].optionsUsed, connections[connectionID].optLenUsed);

        terminateConnection(connectionID);
        /*//clear the connection memory first
        clearBuffer(connectionID);
        clear_nack(connectionID);
        close(connections[connectionID].sockfd);

        //reconnect to remote host
        if (connections[connectionID].remoteIP == nullptr){
            listen(connections[connectionID].localPort, connections[connectionID].protocol, connections[connectionID].sendBufferMaxSize,
                    connections[connectionID].receiveBufferMaxSize, connections[connectionID].optionsUsed,
                    connections[connectionID].optLenUsed);
        }
        else{
            connect(connections[connectionID].remoteIP, connections[connectionID].remotePort, connections[connectionID].protocol,
                    connections[connectionID].sendBufferMaxSize, connections[connectionID].receiveBufferMaxSize,
                    connections[connectionID].optionsUsed, connections[connectionID].optLenUsed);
        }*/
    }
}




