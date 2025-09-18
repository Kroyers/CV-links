//
// Created by Qingyun on 29.11.2019.
//

#ifndef STP_PROTOCOL_TRANSMISSION_H
#define STP_PROTOCOL_TRANSMISSION_H


#include "Option.h"
#include "BufferPacket.h"
#include <stdint-gcc.h>

namespace stp{
    uint32_t calcNextSeqNr(uint32_t seqNr);
    bool initBuffer(int connectionID, uint32_t sendBufSize, uint32_t rcvBufSize);
    void initSeqNr(int connectionID);
    int stpReceive(int connectionID);
    bool stpSend(int connectionID, void* buf, int len, uint8_t flags, Option *options, uint8_t optLen, uint32_t seqNr);
    int readFromRcvBuffer(int connectionID, void* dst, int len, uint32_t seqNr);
    bool retransmit(int connectionID, uint32_t seqNr);
    void retransmitAll(int connectionID);
    int getRcvBufferLen(int connectionID);
    void clearBuffer(int connectionID);
    uint32_t calcLastSeqNr(uint32_t seqNr);
    uint32_t lookUpSeqInRcvBuffer(int connectionID, uint32_t seqNr);

//for testing
    bool writeInRcvRingBuffer(int connectionID, char* ptr, int len, uint32_t seqNr);
    bool writeInSendBuffer(int connectionID, uint32_t seqNr, uint8_t flags, Option options[], uint8_t optLen, char *payloadPtr, uint32_t payloadLength);
    bool readFromSendRingBuffer(int connectionID, uint32_t seqNr, BufferPacket* bufPacketPtr);
    void updateSendRingBufferPointer(int connectionID, uint32_t ackNr);
}


#endif //STP_PROTOCOL_TRANSMISSION_H
