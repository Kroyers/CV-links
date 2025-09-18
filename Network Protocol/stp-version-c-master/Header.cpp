//
// Created by Qingyun on 29.11.2019.
// Edited by Leon
//

#include <iostream>
#include "Header.h"
#include "Connection.h"

#define MAX_3BYTE_NUM 16777215
#define MAX_4BYTE_NUM 4294967295

namespace stp{
    extern Connection connections[MAX_NUMBER_OF_CONNECTIONS];

    //Use this to write sequence number into the header. Return negative number on failure, 0 on success.
    int setSequenceNumber(char header_STP[], uint32_t seqNr){
        //Check expected format
        if (seqNr > MAX_3BYTE_NUM) return -1;
        header_STP[0] = seqNr >> 16;
        header_STP[1] = seqNr >> 8;
        header_STP[2] = seqNr;
        return 0;
    }

    int setAcknowledgementNumber(char header_STP[], uint32_t ackNr){
        //Check expected format
        if (ackNr > MAX_3BYTE_NUM) return -1;
        header_STP[3] = ackNr >> 16;
        header_STP[4] = ackNr >> 8;
        header_STP[5] = ackNr;
        return 0;
    }

//Use this to write acknowledgement number into the header. Return negative number on failure, 0 on success.
    int setWindowSize(char header_STP[], uint32_t wndSize){
        //Check expected format
        //if (wndSize > MAX_4BYTE_NUM) return -1;
        header_STP[6] = wndSize >> 24;
        header_STP[7] = wndSize >> 16;
        header_STP[8] = wndSize >> 8;
        header_STP[9] = wndSize;
        return 0;
    }

//Use this to set flags for the header. Return negative number on failure, 0 on success.
    int setFlags(char header_STP[], uint8_t flags){
        //Check expected format
        if (flags > 15) return -1;         //No valid flag combination
        if (flags == 10) return -1;        //SYN and FIN are set (TODO: Send packet with reset flag)
        //uint8_t flag_shifted = flags << 4;
        uint8_t empty_flag = 0b00000000;
        header_STP[10] = empty_flag;
        header_STP[10] = (header_STP[10] | (flags << 4));
        return 0;

        //Check expected format
        if (flags > 15) return -1;         //No valid flag combination
        if (flags == 10) return -1;        //SYN and FIN are set (TODO: Send packet with reset flag)
        header_STP[10] = header_STP[10] | flags << 4;
        return 0;

    }

//Use this to write the version for the header. Return negative number on failure, 0 on success.
    int setVersion(char header_STP[], uint8_t version){
        //Check expected format
        if (version > 5) return -1;
        header_STP[10] = header_STP[10] | version;
        return 0;
    }

//Use this to write option length into the header. Return negative number on failure, 0 on success.
    int setOptionLength(char header_STP[], uint8_t len){
        header_STP[11] = len;
        return 0;
    }

//Use this to write an option into the slotNr slot of the header. Return negative number on failure, 0 on success.
    bool setOptionSlot(char header_STP[], int slotNr, Option option){
        if (option.type == OPTION_NACK){
            header_STP[12+slotNr] = 0x03;
            header_STP[12+slotNr+1] = (uint8_t)(option.info >> 16);
            header_STP[12+slotNr+2] = (uint8_t)(option.info >> 8);
            header_STP[12+slotNr+3] = (uint8_t)(option.info);
        }
        else if (option.type == OPTION_DATA_COMPRESSION){
            header_STP[12+slotNr] = 0x01;
            header_STP[12+slotNr+1] = (uint8_t)(option.info >> 16);
            header_STP[12+slotNr+2] = (uint8_t)(option.info >> 8);
            header_STP[12+slotNr+3] = (uint8_t)(option.info);
        }
        return true;
    }

//Use this to write optionslots and optionlength into the header. Return negative number on failure, 0 on success.
    int setOptions(char header_STP[], Option* options, uint8_t optLen){
        for (int i = 0; i < optLen; i++) {
            if(!setOptionSlot(header_STP, i, *(options + i))){
                return -1;
            }
        }
        return 0;
    }

//This method transformes a STP-Header-Object into a STP-header-Char-Array.
//Return true on success, false otherwise.
    bool writeHeader(Header header, char header_STP[]){
        for (int i = 0; (i < 12 + 4*header.optLen); i++) {
            header_STP[i] = 0;
        }

        //Filling STP-Header with values
        if (setSequenceNumber(header_STP, header.seqNr) < 0) return false;
        if (setAcknowledgementNumber(header_STP, header.ackNr) < 0) return false;
        if (setWindowSize(header_STP, header.wndSize) < 0) return false;
        if (setFlags(header_STP, header.flags) < 0) return false;
        if (setVersion(header_STP, header.version) < 0) return false;
        if (setOptionLength(header_STP, header.optLen) < 0) return false;
        if (header.optLen > 0){
            if (setOptions(header_STP, header.options, header.optLen) < 0) return false;
        }

        return true;
    }

//Use this to read the sequence number from the header and analyse it. Return seqNr.
    uint32_t getSequenceNumber(char* packet_UDP){
        uint32_t seqNr = 0;
        for (int i=0; i<3; i++){
            seqNr = seqNr << 8;
            seqNr = seqNr | (uint8_t) packet_UDP[i];
        }
        return seqNr;
    }

    bool validateSeqNr(uint32_t seqNr, int connectionID){
        Connection connection = connections[connectionID];
        if (seqNr != connection.seqExpected){
            if (connection.seqExpected <= MAX_3BYTE_NUM - 500 && seqNr > connection.seqExpected + 500){
                return false;
            }
            if (connection.seqExpected > MAX_3BYTE_NUM - 500 && seqNr > 500 - connection.seqExpected){
                return false;
            }
        }
        return true;
    }

    bool validateFlags(uint8_t flags, int context){
        switch(context){
            //invalid first 4 bits
            case CONTEXT_DATA_TRANS:
                if(flags != 0b00000010 && flags != 0b00000000){
                    return false;
                }
                break;
            case CONTEXT_SYN_SENT:
                if(flags != 0b00001100){
                    return false;
                }
                break;
            case CONTEXT_SYN_RECEIVE:
                if(flags != 0b00000010 && flags != 0b00000000){
                    return false;
                }
                break;
            case CONTEXT_SYN_ACK_SENT:
                if(flags != 0b00000100){
                    return false;
                }
                break;
            case CONTEXT_FIN_SENT:
                if(flags != 0b00000110 && flags != 0 && flags != 0b00000010){
                    return false;
                }
                break;
            case CONTEXT_FIN_ACK_SENT:
                if(flags != 0b00000110){
                    return false;
                }
                break;
            case CONTEXT_SYN_EXPECTED:
                if(flags != 0b00001000){
                    return false;
                }
                break;
            default:
                break;
        }
        return true;
    }

//Use this to read the acknowledgement number from the header and analyse it. Return ackNr.
    uint32_t getAcknowledgementNumber(char packet_UDP[]){
        uint32_t ackNr = 0;
        for (int i=3; i<6; i++){
            ackNr = ackNr << 8;
            ackNr = ackNr | (uint8_t) packet_UDP[i];
        }
        return ackNr;
    }

//Use this to read the window size from the header and analyse it. Return wndSize.
    uint32_t getWindowSize(char packet_UDP[]){
        uint32_t wndSize = 0;
        for (int i=6; i<10; i++){
            wndSize = wndSize << 8;
            wndSize = wndSize | (uint8_t) packet_UDP[i];
        }
        return wndSize;
    }

//Use this to read the flags from the header. Return flags.
    uint8_t getFlags(char packet_UDP[]){
        uint8_t flags = 0;
        flags = (uint8_t) packet_UDP[10] >> 4;
        return flags;
    }

//Use this to read the version from the header and analyse it. Return version.
    uint8_t getVersion(char packet_UDP[]){
        uint8_t version;
        version = (uint8_t) packet_UDP[10];
        //Erase the flag field (FLags|Version)
        return (version & 0b00001111);
    }

//Use this to read the Option length from the header and analyse it. Return optLen.
    uint8_t getOptionLen(char packet_UDP[]){
        if(getVersion(packet_UDP) == 3){ //if version is 3
            uint8_t optionlen;
            optionlen = (uint8_t) packet_UDP[11];
            return optionlen;
        }
        return 0;
    }

//Use this to read an option slot from the header. Return option array.
    Option* getOption(char packet_UDP[], uint8_t optLen){
        Option *options = new Option[optLen];
        for (int i = 0; i < optLen; i++) {
            options[i].type = packet_UDP[12+i];
            options[i].info = 0;
            options[i].info |= packet_UDP[12+i+1] << 16;
            options[i].info |= packet_UDP[12+i+2] << 8;
            options[i].info |= packet_UDP[12+i+3];
            options[i].length = 24;
        }
        return options;
    }

//and write information in the header parameter. Return true on success, false otherwise.
    uint8_t readHeader(Header *header_ptr, char *packet_UDP, int context, int connectionID){
        bool success = true;
        uint8_t optLen = *(packet_UDP + 11);
        //Create new Header object to store variables
        //Header header;

        //write bytes into array
        char header_start[12 + 4*optLen];
        for(uint8_t i = 0; i < 12 + 4*optLen; i++){
            header_start[i] = *(packet_UDP+i);
        }

        //Set Header-Object values
        header_ptr->seqNr = getSequenceNumber(header_start);
        if (context != CONTEXT_SYN_SENT && context != CONTEXT_SYN_EXPECTED){
            success = validateSeqNr(header_ptr->seqNr, connectionID);
        }

        if (!success){
            return 2;
        }

        header_ptr->ackNr = getAcknowledgementNumber(header_start);

        header_ptr->wndSize = getWindowSize(header_start);

        header_ptr->flags = getFlags(header_start);

        //check for rst flag
        if(header_ptr->flags << 7 == 128){
            return 3;
        }

        //check for unexpected SYN
        if(header_ptr->flags << 4 >= 128 && context != CONTEXT_SYN_EXPECTED && context != CONTEXT_SYN_SENT){
            return 4;
        }

        success = validateFlags(header_ptr->flags, context);
        if (!success){
            return 1;
        }

        header_ptr->version = getVersion(header_start);

        header_ptr->optLen = getOptionLen(header_start);

        header_ptr->options = getOption(header_start, header_ptr->optLen);

        return 0;
    }
}



