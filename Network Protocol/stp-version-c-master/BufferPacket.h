//
// Created by Qingyun on 01.12.2019.
//

#ifndef STP_PROTOCOL_BUFFERPACKET_H
#define STP_PROTOCOL_BUFFERPACKET_H


#include <stdint-gcc.h>
#include "Option.h"

namespace stp{
    class BufferPacket {
    public:
        uint32_t seqNr;             //SeqNr of this packet
        uint8_t flags;              //A bit map pf flags set in this packet
        Option *options;            //Options used in this packet
        uint8_t optLen;             //Amount of option slot used
        char *payloadPtr;           //Payload of this packet
        uint32_t length;            //Length of the payload
        BufferPacket(uint32_t seqNrInput, uint8_t flagsInput, Option *optionsInput, uint8_t optLenInput, char* payloadPtrInput, uint32_t lengthInput){
            seqNr = seqNrInput;
            flags = flagsInput;
            payloadPtr = payloadPtrInput;
            length = lengthInput;
            options = optionsInput;
            optLen = optLenInput;
        }
    };
}

#endif //STP_PROTOCOL_BUFFERPACKET_H
