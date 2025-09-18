//
// Created by Qingyun on 29.11.2019.
// This class contains all information stored in a header
// and help us to implement header methods
//

#ifndef STP_PROTOCOL_HEADER_H
#define STP_PROTOCOL_HEADER_H


#include <stdint-gcc.h>
#include "Option.h"

namespace stp{
    class Header{
    public:
        uint32_t seqNr;             //Sequence Number should be an integer between 0 and 2^24
        uint32_t ackNr;             //Acknowledgement Number should be an integer between 0 and 2^24
        uint32_t wndSize;           //Window size should be an integer etween 0 and 0^32
        uint8_t flags;              //Combine all flags into one integer, use LSBs for the flags:0000|SYN|ACK|FIN|RST|.
        // The developer of Header function may decide if this is appropriate.
        uint8_t version;            //Currently only 0 to 5 are defined.
        uint8_t optLen;             //Must be the same as the amount of Option slots we use.
        Option *options;            //An array of all options for this packet.
        Header(uint32_t seqNrInput, uint32_t ackNrInput, uint32_t wndInput, uint8_t flagsInput, uint8_t versionInput, uint8_t optLenInput, Option *optionsInput){
            seqNr = seqNrInput;
            ackNr = ackNrInput;
            flags = flagsInput;
            wndSize = wndInput;
            version = versionInput;
            optLen = optLenInput;
            options = optionsInput;
        }
        Header(){
            seqNr = 0;
            ackNr = 0;
            flags = 0;
            wndSize = 0;
            version = 0;
            optLen = 0;
            options = nullptr;
        }
    };

    uint8_t readHeader(Header *header_ptr, char *packet_UDP, int context, int connectionID);

    bool writeHeader(Header header, char *buf);
}
#endif //STP_PROTOCOL_HEADER_H
