//
// Created by Qingyun on 29.11.2019.
//

#ifndef STP_PROTOCOL_CONNECTION_H
#define STP_PROTOCOL_CONNECTION_H

#include "Option.h"
#include "Interface.h"
#include "CongestionControl.h"
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include "BufferPacket.h"
#include <vector>
#include "Nack.h"

#define FIN_ACK_TERMINATE_MODE 0
#define FIN_ACK_WAIT_MODE 1

namespace stp{
    int terminateConnection(int connectionID);

    int accept(int connectionID);

    int connect(char* ip, char* port, uint8_t protocol, uint32_t sendWndInput, uint32_t rcvWndInput, Option* options, uint8_t optLen);

    int stpClose(int connectionID);

    int sendFinAck(int connectionID, int mode);

    int listen(char* localPort, uint8_t protocol, uint32_t sendWndInput, uint32_t rcvWndInput, Option* options, uint8_t optLen);

    void reset(int connectionID);

    class Connection {
    public:
        //Shows if this connection is currently used or not
        bool isFree = true;
        //Local addrinfo of the connection
        struct addrinfo *locAddrInfo;
        //Remote addrinfo of the connection
        struct addrinfo *remoteAddrInfo;
        //The unacknowledged seqNr
        uint32_t seqUnack = 0;
        //The sequence number we use for the next new packet
        uint32_t seqNext = 0;
        //The sequence number we are expecting
        uint32_t seqExpected = 0;
        //The sequence number we can acknowledge
        uint32_t seqToack = 0;
        //Number of bytes we are still able to buffer as payload
        uint32_t rcvWnd = 3000;
        //Number of bytes the other side can still receive as payload
        uint32_t sendWnd = 3000;
        //The seqNr user has just read
        uint32_t seqLastRead = 0;
        //Address information of the destination
        struct sockaddr *dst;
        //Socket of the current connection
        int sockfd;
        //Length of the address
        socklen_t dstLen = sizeof(sockaddr_storage);
        //A pointer to dstLen
        socklen_t *dstLenPtr = &dstLen;
        //Current phase of connection, should be set to the according context
        int connectionPhase = -1;
        //Stores if we have received a FIN message
        bool finRcved = false;
        //Shows if connection is terminated
        bool connectionTerminated = false;
        //Shows the version of protocol used in this connection
        uint8_t versionUsed = 3;
        //An array of options that are used in the connection
        Option* optionsUsed;
        //Number of options in the array
        uint8_t optLenUsed = 0;
        //Shows if cc is used by the host
        bool congestionControlUsed = false;
        //Shows if NACK is used in the connection
        bool NACKUsed = false;
        //Shows if dc is used in the connection
        bool dataCompressionUsed = false;
        //The code for algorithm which is used for compression, 0 is none is used
        uint8_t dataCompressionAlgo = 0;
        //Our ring buffer for send
        BufferPacket **sendRingBuffer = new BufferPacket*[MAX_RINGBUFFER_SLOT];
        //Number of bytes already used in the send buffer
        uint32_t sendBufferUsed = 0;
        //Head of our send ring buffer, this is the next free slot!
        uint16_t sendBufferHead = 0;
        //Tail of our send ring buffer, this is the last unread slot!
        uint16_t sendBufferTail = 0;
        //Maximum size of send buffer
        uint32_t sendBufferMaxSize = MAX_BUFFER_SIZE;
        //Rcv Ring buffer
        BufferPacket **rcvRingBuffer = new BufferPacket*[MAX_RINGBUFFER_SLOT];
        //Number of bytes already used in the receive buffer
        uint32_t receiveBufferUsed = 0;
        //Head of our receive ring buffer, this is the next free slot!
        uint16_t receiveBufferHead = 0;
        //Tail of our receive ring buffer, this is the last unread slot!
        uint16_t receiveBufferTail = 0;
        //Maximum size of receive buffer
        uint32_t receiveBufferMaxSize = MAX_BUFFER_SIZE;
        //Congestion window
        uint16_t cWnd = MIN_CONGESTION_WINDOW;
        //Congestion threshold
        uint16_t cThreshold = STARTING_THRESHOLD;
        //congestion counter
        uint16_t cCounter = 0;
        //Nack vector
        std::vector<uint32_t> nack_vector{};
        //This is the buffer used for accepting UDP Packet
        char* workingBuffer;

        //Timer:
        //Congestion Timer
        uint16_t cTimer = NOT_EXISTING_TIMER;
        //Global Timer
        uint16_t globalTimer = NOT_EXISTING_TIMER;
        //Heartbeat Timer
        uint16_t heartbeatTimer = NOT_EXISTING_TIMER;
        //Retransmitt Timer
        uint16_t retransTimer = NOT_EXISTING_TIMER;

        //Running time for timers:
        uint16_t globalTime = GLOBAL_TIMEOUT_MILLISEC;
        uint16_t heartbeatTime = HEARTBEAT_TIMEOUT_MILLISEC;
        uint16_t retransTime = 2000;

        //Reset Timer:
        //Global Timer reset
        bool resetGlobalTimer = false;
        //Heartbeat Timer reset
        bool resetHeartbeatTimer = false;
        //Retransmitt Timer reset
        bool resetRetransTimer = false;

        //
        int nCounter = 0;
        //store localPort for reset
        char* localPort = nullptr;
        //store remotePort for reset
        char* remotePort = nullptr;
        //store remote IP for reset
        char* remoteIP = nullptr;
        //Indicates current Nack phase, 0 means normal operation,
        //1 means a missing packet is detected
        //2 means our vector is full a NACK must be sent now
        //3 means we sent a NACK but we haven't received the requested packets
        int nackPhase = 0;
        //Input Buffer used to store user input for payload batching
        char inputBuffer[65535];
        //amount of bytes used in Inputbuffer
        int inputBufferUsed = 0;
        //Protocol used in this connection 6 means IPv6, 4 means IPv4, anything else is interpreted as IPv6
        uint8_t protocol = 6;


        Connection(){
            dst = new sockaddr;
        }
    };
}
#endif //STP_PROTOCOL_CONNECTION_H
