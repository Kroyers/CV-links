#include "Header.h"
#include "Transmission.h"
#include <iostream>

namespace stp{
    //Print function for testing
    void printHeader(char header_STP[]){
        std::cout << "Header = ";
        for(int j=0; j<16; j++){
            for(int i=7; i >= 0; i--){
                std::cout << (((header_STP[j] >> i) % 2) ? '1' : '0');
            }
            std::cout << " ";
        }
        std::cout << "\n";
    }

//Print function for testing
    void printHeaderObject(Header *header_ptr){
        std::cout << "Seq: " << header_ptr -> seqNr  << std::endl;
        std::cout << "Ack: " << header_ptr -> ackNr << std::endl;
        std::cout << "Wnd: " << header_ptr -> wndSize << std::endl;
        std::cout << "Flags: " << (uint32_t) header_ptr -> flags << std::endl;
        std::cout << "Version: " << (uint32_t)header_ptr -> version  << std::endl;
        std::cout << "Optlen: " << (uint32_t)header_ptr -> optLen  << std::endl;
        std::cout << "Opttype: " << (uint32_t)header_ptr -> options ->type  << std::endl;
        std::cout << "Optinfo: " << (uint32_t)header_ptr -> options ->info  << std::endl;
    }

    void testRingBufferWrite(){
        initBuffer(1,1000, 1000);

        char* temp = (char*)malloc(12*sizeof(char));
        for(uint8_t i = 0; i < 12; i++){
            *(temp+i) = i;
        }
        bool write = writeInRcvRingBuffer(0, temp, 12, 0);
        std::cout << "Write Success: " << write << std::endl;

        for(uint8_t i = 0; i < 12; i++){
            *(temp+i) = (i + 12);
        }
        write = writeInRcvRingBuffer(0, temp, 12, 1);
        std::cout << "Write Success: " << write << std::endl;

        char* readtest = (char*)malloc(12*sizeof(char));
        int testbytesread = readFromRcvBuffer(0, readtest, 12, 0);
        std::cout << "Bytes read: " << testbytesread << std::endl;
        for(uint8_t j = 0; j < 12; j++){
            std::cout << (uint32_t)*(readtest+j) << std::endl;
        }

        testbytesread = readFromRcvBuffer(0, readtest, 14, 1);
        std::cout << "Bytes read: " << testbytesread << std::endl;
        for(uint8_t j = 0; j < 12; j++){
            std::cout << (uint32_t)*(readtest+j) << std::endl;
        }

        testbytesread = readFromRcvBuffer(0, readtest, 12, 2);
        std::cout << "Bytes read: " << testbytesread << std::endl;
        for(uint8_t j = 0; j < 12; j++){
            std::cout << (uint32_t)*(readtest+j) << std::endl;
        }
    }

    void testRingBufferSend(){
        initBuffer(0,30,200);
        Option options[0];
        char* payloadPtr = (char*)malloc(10*sizeof(char));
        BufferPacket *bufPacketPtr = new BufferPacket(0, 0, options, 0, nullptr, 0);
        int write;
        int read;



        for (uint8_t i = 0; i < 12; i++)
        {
            for(uint8_t j = 0; j < 10; j++){
                *(payloadPtr + j) = j + i;
            }
            write = writeInSendBuffer(0, i, 0, options, 0, payloadPtr, 10);
            std::cout << "Write Success: " << write << std::endl;
        }

        read = readFromSendRingBuffer(0, 0 , bufPacketPtr);
        std::cout << "Read Success: " << read << std::endl;
        std::cout << "SeqNr: " << bufPacketPtr->seqNr << std::endl;
        std::cout << "Flags: " << (uint32_t)bufPacketPtr->flags << std::endl;
        std::cout << "OptLen: " << (uint32_t)bufPacketPtr->optLen << std::endl;
        std::cout << "Length: " << bufPacketPtr->length << std::endl;
        for (int k = 0; k < bufPacketPtr->length; ++k) {
            std::cout << "Payload at " << k << " : " << (uint32_t)*(bufPacketPtr->payloadPtr + k) << std::endl;
        }
        read = readFromSendRingBuffer(0, 1 , bufPacketPtr);
        std::cout << "Read Success: " << read << std::endl;
        std::cout << "SeqNr: " << bufPacketPtr->seqNr << std::endl;
        std::cout << "Flags: " << (uint32_t)bufPacketPtr->flags << std::endl;
        std::cout << "OptLen: " << (uint32_t)bufPacketPtr->optLen << std::endl;
        std::cout << "Length: " << bufPacketPtr->length << std::endl;
        for (int k = 0; k < bufPacketPtr->length; ++k) {
            std::cout << "Payload at " << k << " : " << (uint32_t)*(bufPacketPtr->payloadPtr + k) << std::endl;
        }
        read = readFromSendRingBuffer(0, 2 , bufPacketPtr);
        std::cout << "Read Success: " << read << std::endl;
        std::cout << "SeqNr: " << bufPacketPtr->seqNr << std::endl;
        std::cout << "Flags: " << (uint32_t)bufPacketPtr->flags << std::endl;
        std::cout << "OptLen: " << (uint32_t)bufPacketPtr->optLen << std::endl;
        std::cout << "Length: " << bufPacketPtr->length << std::endl;
        for (int k = 0; k < bufPacketPtr->length; ++k) {
            std::cout << "Payload at " << k << " : " << (uint32_t)*(bufPacketPtr->payloadPtr + k) << std::endl;
        }
        updateSendRingBufferPointer(0, 3);
        for (uint8_t i = 6; i < 9; i++)
        {
            write = writeInSendBuffer(0, i, 0, options , 0, payloadPtr, 10);
            std::cout << "Write Success: " << write << std::endl;
        }
    }
}

int main(){
    /*Option nackOption;
    nackOption.type = OPTION_NACK;
    nackOption.info = 0;
    nackOption.length = 0;

    Option dcOption;
    dcOption.type = OPTION_DATA_COMPRESSION;
    dcOption.info = COMP_ALGO_HUFFMAN << 16;
    dcOption.length = 8;

    Option options[1] = {dcOption};

    //Create test header object
    Header test_header;
    test_header.seqNr = (uint32_t) 128;
    test_header.ackNr = (uint32_t) 2;
    test_header.wndSize = (uint32_t) 4;
    test_header.flags = (uint8_t) 1;
    test_header.version = (uint8_t) 3;
    test_header.optLen = (uint8_t)1;
    test_header.options = options;


    //Test writeHeader
    auto *msg = (char*)malloc((20)*sizeof(char));
    writeHeader(test_header, msg);
    printHeader(msg);

    //Test readHeader
    Header* header = new Header;
    readHeader(header, msg, CONTEXT_SYN_SENT);
    printHeaderObject(header);*/

    stp::testRingBufferWrite();
    stp::testRingBufferSend();

    return 0;
}


