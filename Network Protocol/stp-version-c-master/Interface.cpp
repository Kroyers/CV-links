#include "Interface.h"
#include "Transmission.h"
#include "Connection.h"
#include <string.h>
#include "Timers.h"


namespace stp{
    Connection connections[MAX_NUMBER_OF_CONNECTIONS];

//Sends a PAYLOAD PACKET, this is a INTERFACE FOR USERS and MUST NOT be used in methods like connect, close etc
    int send(int connectionID, void* buf, int len){
        if (connections[connectionID].isFree || len < 0){
            std::cerr << "Trying to write into a non-existent connection, please try to reconnect" << std::endl;
            sleep(10);
            return -2;
        }

        if (len > 65507){
            std::cerr << "Packet size over MTU of STP packet, please try again with shorter length." << std::endl;
            return -1;
        }

        Option* nackOpt;
        uint8_t optLen = 0;
        if (connections[connectionID].NACKUsed && (connections[connectionID].nackPhase == 1)){
            nackOpt = nack_list(connectionID);
            optLen++;
        }

        return stpSend(connectionID, buf, len,0, nackOpt, optLen, connections[connectionID].seqNext);

        //std::cerr << "seqUnack: " << (uint32_t)connections[connectionID].seqUnack << " seqNext: " << (uint32_t)connections[connectionID].seqNext << std::endl;

        /*if (connections[connectionID].seqUnack == connections[connectionID].seqNext || clacLastSeqNr(connections[connectionID].seqNext) == connections[connectionID].seqUnack){
            //std::cerr << "sending for every packet is acked" << std::endl;
            return stpSend(connectionID, buf, len,0, nackOpt, optLen, connections[connectionID].seqNext);
        }

        if (connections[connectionID].inputBufferUsed + len <= 65535){
            memcpy(connections[connectionID].inputBuffer + connections[connectionID].inputBufferUsed, buf, len);
            connections[connectionID].inputBufferUsed += len;
        }

        if (connections[connectionID].inputBufferUsed >= 1440){
            //std::cerr << "sending from input buffer" << std::endl;
            connections[connectionID].inputBufferUsed = 0;
            return stpSend(connectionID, connections[connectionID].inputBuffer, connections[connectionID].inputBufferUsed,0, nackOpt, optLen, connections[connectionID].seqNext);
        }*/

        return false;
    }

//Receives one packet from UDP and writes content into buf, note that content not read will be buffered until next
//receive call. returns the actual length that has been read.
    int receive(int connectionID, void *buf, size_t len){
        if (connections[connectionID].isFree){
            std::cerr << "Trying to read from a non-existent connection, please try to reconnect" << std::endl;
            sleep(10);
            return -2;
        }
        stpReceive(connectionID);

        int available = readFromRcvBuffer(connectionID, buf, len, connections[connectionID].seqLastRead + 1);
        if (available >= 0){
            connections[connectionID].seqLastRead++;
        }
        //std::cerr << "available: " << available << std::endl;
        return available;
    }

    void startHearbeatTimer(int connectionID){
        timer(HEARTBEAT_TIMEOUT_MILLISEC, connectionID, HEARTBEAT_TIMER);
    }

    //Use this to send a heartbeat packet
    void heartBeat(int connectionID){
        Option* options = nullptr;
        uint8_t optLenInput = 0;
        uint8_t flags = 0;
        char empty_array[0];
        stpSend(connectionID, empty_array, 0, flags, options, optLenInput, connections[connectionID].seqNext);
        startHearbeatTimer(connectionID);
    }

    //Starts the Global timeout. Return negative value on failure, 0 on success.
    void startGlobalTO(int connectionID){
        timer(GLOBAL_TIMEOUT_MILLISEC, connectionID, GLOBAL_TIMER);
    }

    void startTimers(int connectionID){
        startGlobalTO(connectionID);
        startHearbeatTimer(connectionID);
    }
}



