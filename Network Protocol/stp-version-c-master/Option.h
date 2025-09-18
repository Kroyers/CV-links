//
// Created by Qingyun on 29.11.2019.
// This class defines the option slots we use to extend our header.
//

#ifndef STP_PROTOCOL_OPTION_H
#define STP_PROTOCOL_OPTION_H


#include <stdint-gcc.h>

namespace stp{
    #define OPTION_DATA_COMPRESSION 1
    #define OPTION_CONGESTION_CONTROL 2
    #define OPTION_NACK 3
    #define NO_OPTIONS 0xFF
    #define COMP_ALGO_HUFFMAN 0x05
    #define COMP_ALGO_LZW 0x02

    class Option {
    public:
        uint8_t type = NO_OPTIONS;               //The type of the option slot, must always be written into the first byte.
        uint32_t info = 0;              //The information we provide in this option slot.
        uint8_t length = 0;             //Number of bits used in this option slot. This is declared for easier padding.

    };
}




#endif //STP_PROTOCOL_OPTION_H
