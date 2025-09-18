//
// Created by Qingyun on 29.11.2019.
//


#include "Transmission.h"
#include "Interface.h"
#include "Header.h"
#include "CongestionControl.h"
#include "Compression.h"
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <random>
#include <string.h>
#include <iostream>
#include <chrono>
#include <sys/socket.h>
#include "Connection.h"
#include "Timers.h"
#include <thread>
#include <mutex>


namespace stp{
    extern Connection connections[MAX_NUMBER_OF_CONNECTIONS];
    std::mutex mu;

//Generally calculates the next sequence number in the sequence number room
//ALWAYS USE THIS FUNCTION IF CALCULATION IS NEEDED
    uint32_t calcNextSeqNr(uint32_t seqNr){
        return (seqNr + 1) % MAX_SEQ_NR;
    }

    uint32_t calcLastSeqNr(uint32_t seqNr){
        if (seqNr == 0){
            return MAX_SEQ_NR - 1;
        }
        else {
            return seqNr - 1;
        }
    }

//Mode 0: initialize sequence number
//Mode 1: claculate next sequence number
//seqNr: current seqNr
//Returns the result.
    uint32_t calcSequenceNumber(int mode, uint32_t seqNr){
        uint32_t nxtSeqNr = seqNr;
        //If mode is 0, generate a new integer number with uniform distribution
        if (mode == INIT_SEQ_MODE){
            typedef std::chrono::high_resolution_clock myclock;
            myclock::time_point beginning = myclock::now();
            myclock::duration d = myclock::now() - beginning;
            unsigned seed2 = d.count();
            std::default_random_engine generator;
            std::uniform_int_distribution<int> distribution(0,MAX_SEQ_NR);
            generator.seed (seed2);
            nxtSeqNr = distribution(generator);
        }
        else if(mode == NEXT_SEQ_MODE){
            nxtSeqNr = calcNextSeqNr(seqNr);
        }
        else {
            return 0;
        }
        return nxtSeqNr;
    }

//initialise sequence number
    void initSeqNr(int connectionID){
        connections[connectionID].seqNext = calcSequenceNumber(INIT_SEQ_MODE, 0);
    }

//calculates the next slot number for ring buffer
    uint16_t ringBufferNext(int mode, uint16_t slotNr){
        if (mode == SEND_BUFFER_MODE){
            return (slotNr + 1) % MAX_RINGBUFFER_SLOT;
        }
        else if(mode == RECEIVE_BUFFER_MODE){
            return (slotNr + 1) % MAX_RINGBUFFER_SLOT;
        }
        else {
            return false;
        }
    }

//Check if a ring buffer is full:
//mode 0: send buffer; mode 1: receive buffer
//return true if the next slot is free, false if otherwise
    bool rBufferIsNextFree(int connectionID, int mode){
        if (mode == SEND_BUFFER_MODE){
            return ringBufferNext(SEND_BUFFER_MODE, connections[connectionID].sendBufferHead) != connections[connectionID].sendBufferTail;
        }
        else if (mode == RECEIVE_BUFFER_MODE){
            return ringBufferNext(RECEIVE_BUFFER_MODE, connections[connectionID].receiveBufferHead) != connections[connectionID].receiveBufferTail;
        }
        else {
            return false;
        }
    }

//Check if a ring buffer has any content:
//mode 0: send buffer; mode 1:receive buffer
//return true if buffer has content, false if otherwise
    bool rBufferHasContent(int connectionID, int mode){
        if (mode == SEND_BUFFER_MODE){
            return connections[connectionID].sendBufferTail != connections[connectionID].sendBufferHead;
        }
        else if (mode == RECEIVE_BUFFER_MODE){
            return connections[connectionID].receiveBufferTail != connections[connectionID].receiveBufferHead;
        }
        else {
            return false;
        }
    }

//Updates the send buffer pointer after an ackNr has been received to free up space in send buffer
    void updateSendRingBufferPointer(int connectionID, uint32_t ackNr){
        bool flag = false;
        while (rBufferHasContent(connectionID, SEND_BUFFER_MODE)){
            BufferPacket *current = connections[connectionID].sendRingBuffer[connections[connectionID].sendBufferTail];
            if (current->seqNr == ackNr){
                flag = true;
            }
            delete[](current->payloadPtr);
            connections[connectionID].sendBufferUsed -= (current->length + BUFFER_PACKET_OVERHEAD);
            connections[connectionID].sendBufferTail = ringBufferNext(SEND_BUFFER_MODE, connections[connectionID].sendBufferTail);
            delete(current);
            if (flag){
                return;
            }
        }
    }

    bool validateSeqNr(int connectionID, uint32_t seqNr){
        //std::cerr << "seqNr: " << seqNr << "Expected: " << connections[connectionID].seqExpected << std::endl;
        if (connections[connectionID].seqExpected < MAX_SEQ_NR - 500){
            return connections[connectionID].seqExpected  + 500 > seqNr && seqNr >= connections[connectionID].seqExpected;
        }
        else{
            return (seqNr >= connections[connectionID].seqExpected || seqNr < connections[connectionID].seqExpected  + 500 - MAX_SEQ_NR);
        }
    }

//Validates an acknowledgement number. Return negative value on failure, 0 on success.
    bool validateAckNr(int connectionID, uint32_t ackNr){
        return ackNr >= connections[connectionID].seqUnack && ackNr < connections[connectionID].seqNext;
    }

//Validate and update AckNr on this STP instance. Also frees up ring buffer and buffer needed for the payloads
    void updateAckNr(int connectionID, uint32_t ackNr){
        if (!validateAckNr(connectionID, ackNr)){
            return;
        }
        updateSendRingBufferPointer(connectionID, ackNr);
        connections[connectionID].seqUnack = calcNextSeqNr(ackNr);
    }

//Updates the window size after new messages received from other STP instance
    void updateSendWndSize(int connectionID, uint32_t wndSize){
        connections[connectionID].sendWnd = wndSize;
        //std::cerr << "SendWnd updated to: " << connections[connectionID].sendWnd << std::endl;
    }

//Updates rcv window
//mode 0: add to; mode 1: subtract from
    void updateRcvWndSize(int connectionID, uint32_t change, int mode){
        if (mode == RCV_WND_ADD_MODE){
            if (connections[connectionID].rcvWnd + change <= connections[connectionID].receiveBufferMaxSize){
                connections[connectionID].rcvWnd += change;
                return;
            }
        }
        else if (mode == RCV_WND_SUBTRACT_MODE){
            if (change <= connections[connectionID].rcvWnd){
                connections[connectionID].rcvWnd -= change;
                return;
            }
        }
        else{
        }
    }

//Initialise the send buffer, return true on success and false on failure
    bool initSendBuffer(int connectionID, uint32_t size){
        connections[connectionID].sendBufferMaxSize = size;
        return true;
    }

//Initialise the send buffer, return true on success and false on failure
    bool initRcvBuffer(int connectionID, uint32_t size){
        connections[connectionID].receiveBufferMaxSize = size;
        connections[connectionID].rcvWnd = size;
        return true;
    }

//Initialises receive and send buffer, note that only ring buffer is received, data buffer is reserved dynamically.
    bool initBuffer(int connectionID, uint32_t sendBufSize, uint32_t rcvBufSize){
        bool flag = true;
        flag = flag && initSendBuffer(connectionID, sendBufSize);
        if (!flag){
            return false;
        }
        flag = flag && initRcvBuffer(connectionID, rcvBufSize);
        return flag;
    }

//returns false if buffer does not have enough place, DOES NOT CHECK if ring buffer is free
//mode: 0: send buffer, 1: receive buffer
    bool isBufferFree(int connectionID, int mode, uint32_t size){
        if (mode == SEND_BUFFER_MODE){
            //return true;
            return (connections[connectionID].sendBufferMaxSize - connections[connectionID].sendBufferUsed) > size;
        }
        else if (mode == RECEIVE_BUFFER_MODE){
            return (connections[connectionID].receiveBufferMaxSize - connections[connectionID].receiveBufferUsed) > size;
        }
        else {
            return false;
        }
    }

//insert a pointer to Bufferpacket into receive ring buffer, return false if failure
    bool writeInRcvRingBuffer(int connectionID, char* ptr, int len, uint32_t seqNr){
        //If ring buffer is full, return false
        if (!rBufferIsNextFree(connectionID, RECEIVE_BUFFER_MODE)){
            std::cerr << "Ring Full" << std::endl;
            return false;
        }

        uint32_t dataBufferUsage = len + BUFFER_PACKET_OVERHEAD;

        //If we will go over the data buffer size, return false
        if (!isBufferFree(connectionID, RECEIVE_BUFFER_MODE, dataBufferUsage)){
            std::cerr << "Max Buffer size reached" << std::endl;
            return false;
        }

        //Copy data and create new Bufferpacket
        char* backUpPtr = new char[len];
        memcpy(backUpPtr, ptr, len);
        auto *bufferPacketPtr = new BufferPacket(seqNr, 0, nullptr, 0, backUpPtr, len);

        //Store Bufferpacket in ring buffer and update head
        connections[connectionID].rcvRingBuffer[connections[connectionID].receiveBufferHead] = bufferPacketPtr;
        connections[connectionID].receiveBufferHead = ringBufferNext(RECEIVE_BUFFER_MODE, connections[connectionID].receiveBufferHead);
        connections[connectionID].receiveBufferUsed += len;

        //std::cerr << "Wrote seqNr: " << seqNr << " In recvBuffer" << std::endl;
        return true;
    }

//insert a pointer to Bufferpacket into send ring buffer, return false if failure
    bool insertInSendRingBuffer(int connectionID, BufferPacket* ptr){
        //If ring buffer is full, return false
        if (!rBufferIsNextFree(connectionID, SEND_BUFFER_MODE)){
            std::cerr << "Ring is full" << std::endl;
            return false;
        }

        uint32_t dataBufferUsage = ptr->length + BUFFER_PACKET_OVERHEAD;

        //If we will go over the data buffer size, return false
        if (!isBufferFree(connectionID, SEND_BUFFER_MODE, dataBufferUsage)){
            std::cerr << "Buffer size capped" << std::endl;
            return false;
        }

        //Wrtie pointer and update head, buffer used size
        connections[connectionID].sendRingBuffer[connections[connectionID].sendBufferHead] = ptr;
        connections[connectionID].sendBufferHead = ringBufferNext(SEND_BUFFER_MODE, connections[connectionID].sendBufferHead);
        connections[connectionID].sendBufferUsed += dataBufferUsage;
        return true;
    }

//Writes payload into receive buffer if possible, updates pointers in the receive buffer as well
//Return false on failure, true on success.
    bool writeInSendBuffer(int connectionID, uint32_t seqNr, uint8_t flags, Option options[], uint8_t optLen, char *payloadPtr, uint32_t payloadLength){
        char *payloadBackUpPtr = new char[payloadLength];
        memcpy(payloadBackUpPtr, payloadPtr, payloadLength);
        auto *bufPacketPtr = new BufferPacket(seqNr, flags, options, optLen, payloadBackUpPtr, payloadLength);
        if (!insertInSendRingBuffer(connectionID, bufPacketPtr)){
            return false;
        }
        return true;
    }

//Look up the index of buffer slot where the packet with seqNr lies. return the index.
    uint32_t lookUpSeqInSendBuffer(int connectionID, uint32_t seqNr) {
        bool found = false;
        uint16_t current = connections[connectionID].sendBufferTail;
        while (current != connections[connectionID].sendBufferHead && !found) {
            if (connections[connectionID].sendRingBuffer[current]->seqNr == seqNr) {
                found = true;
            }
            else{
                current = ringBufferNext(SEND_BUFFER_MODE, current);
            }
        }

        if (found){
            return current;
        }
        else{
            //return a invalid slot index if not found
            return 0xFFFFFFFF;
        }
    }

//Look up the index of buffer slot where the packet with seqNr lies. return the index.
    uint32_t lookUpSeqInRcvBuffer(int connectionID, uint32_t seqNr) {
        bool found = false;
        uint16_t current = connections[connectionID].receiveBufferTail;
        while (current != connections[connectionID].receiveBufferHead && !found) {
            //std::cerr << "Tail: " << connections[connectionID].receiveBufferTail << "Head: " << connections[connectionID].receiveBufferHead
            //<< "Cur: " << (uint32_t)current << std::endl;
            if (connections[connectionID].rcvRingBuffer[current]->seqNr == seqNr) {
                found = true;
            }
            else{
                current = ringBufferNext(RECEIVE_BUFFER_MODE, current);
            }
        }
        if (found){
            return current;
        }
        else{
            //return a invalid slot index if not found
            return 0xFFFFFFFF;
        }
    }

//Returns how many bytes we have in our receive buffer
    int getRcvBufferLen(int connectionID){
        return connections[connectionID].receiveBufferUsed;
    }

//Updates the send buffer pointer after an ackNr has been received to free up space in send buffer
    void updateRcvRingBufferPointer(int connectionID, uint32_t seqNr){
        //std::cerr << "Updating Receive window to seqNr: " << seqNr << std::endl;
        bool flag = false;
        while (rBufferHasContent(connectionID, RECEIVE_BUFFER_MODE)){
            BufferPacket *current = connections[connectionID].rcvRingBuffer[connections[connectionID].receiveBufferTail];
            if (current->seqNr == seqNr){
                flag = true;
            }
            delete(current->payloadPtr);
            connections[connectionID].receiveBufferUsed -= (current->length + BUFFER_PACKET_OVERHEAD);
            connections[connectionID].receiveBufferTail = ringBufferNext(RECEIVE_BUFFER_MODE, connections[connectionID].receiveBufferTail);
            delete(current);
            if (flag){
                return;
            }
        }
    }

//Read and removes len bytes from ring buffer and free up buffer for their payload
    int readFromRcvBuffer(int connectionID, void* dst, int len, uint32_t seqNr){
        //std::cerr << "Trying to read Seq: " << seqNr << "\n";// << std::endl;
        //std::cerr << "SeqLastRead: " << connections[connectionID].seqLastRead << std::endl;
        //std::cerr << "SeqExpected: " << connections[connectionID].seqExpected << std::endl;
        if (!rBufferHasContent(connectionID, RECEIVE_BUFFER_MODE)){
            //std::cerr << "Ring buffer is empty" << std::endl;
            return -1;
        }

        uint32_t slotNr = lookUpSeqInRcvBuffer(connectionID, seqNr);
        if (slotNr == 0xFFFFFFFF){
            //std::cerr << "target package not found" << std::endl;
            return -1;
        }

        BufferPacket *bufferPacketPtr = connections[connectionID].rcvRingBuffer[slotNr];

        if (bufferPacketPtr->length > len){
            std::cerr << "Package has more data than your buffer" << std::endl;
            return -1;
        }

        int actualLen = bufferPacketPtr->length;

        memcpy(dst, bufferPacketPtr->payloadPtr, actualLen);

        //std::cerr << "Tail before update: " << connections[connectionID].receiveBufferTail << std::endl;
        //connections[connectionID].receiveBufferTail = ringBufferNext(RECEIVE_BUFFER_MODE, connections[connectionID].receiveBufferTail);
        //std::cerr << "Tail after update: " << connections[connectionID].receiveBufferTail << std::endl;
        updateRcvWndSize(connectionID, actualLen, RCV_WND_ADD_MODE);
        connections[connectionID].receiveBufferUsed -= actualLen;
        updateRcvRingBufferPointer(connectionID, seqNr);
        return actualLen;
    }

//Clears send buffer
    void clearSendBuffer(int connectionID){
        //std::cerr << "Clearing send buffer" << std::endl;
        if (connections[connectionID].sendBufferUsed > 0){
            updateSendRingBufferPointer(connectionID, calcLastSeqNr(connections[connectionID].seqNext));
        }
        connections[connectionID].sendBufferUsed = 0;
        //delete[](connections[connectionID].sendRingBuffer);
    }

//clears receive buffer
    void clearRcvBuffer(int connectionID){
        //std::cerr << "Clearing recv buffer" << std::endl;
        if (connections[connectionID].receiveBufferUsed > 0){
            updateRcvRingBufferPointer(connectionID, calcLastSeqNr(connections[connectionID].seqExpected));
        }
        connections[connectionID].receiveBufferUsed = 0;
        //delete[](connections[connectionID].rcvRingBuffer);
    }

//clears both buffer
    void clearBuffer(int connectionID){
        clearRcvBuffer(connectionID);
        clearSendBuffer(connectionID);
    }

    bool flagOp(int connectionID, uint8_t flags){
        if(flags == FIN_FLAG){
            if (connections[connectionID].connectionPhase == CONTEXT_DATA_TRANS){
                connections[connectionID].finRcved = true;
            }
            else if(connections[connectionID].connectionPhase == CONTEXT_FIN_SENT){
                mu.unlock();
                std::cerr << "Synchronous termination detected" << std::endl;
                sendFinAck(connectionID, FIN_ACK_WAIT_MODE);
            }
        }

        if (flags == FIN_ACK_FLAG){
            if(connections[connectionID].connectionPhase == CONTEXT_FIN_SENT){
                sendFinAck(connectionID, FIN_ACK_TERMINATE_MODE);
            }
            else if(connections[connectionID].connectionPhase == CONTEXT_FIN_ACK_SENT){
                sendFinAck(connectionID, FIN_ACK_TERMINATE_MODE);
            }
            else{
                return -1;
            }
        }

        return true;
    }

    uint8_t isCompressionUsed(Option *options, uint8_t optLen){
        for (int i = 0; i < optLen; i++) {
            if ((options + i)->type == OPTION_DATA_COMPRESSION){
                return ((options + i)->info & 0x00FF0000)>>16;
            }
        }
        return false;
    }

    bool isNACKUsed(Option *options, uint8_t optLen){
        for (int i = 0; i < optLen; i++) {
            if ((options + i)->type == OPTION_NACK){
                return true;
            }
        }
        return false;
    }

    bool supportsCompressionAlgo(int connectionID, uint8_t algoCode){
        Option dcOption;
        bool found = false;
        int cur = 0;

        while (cur < connections[connectionID].optLenUsed && !found) {
            if ((connections[connectionID].optionsUsed + cur)->type == OPTION_DATA_COMPRESSION){
                dcOption = *(connections[connectionID].optionsUsed + cur);
                found = true;
            }
            else{
                cur ++;
            }
        }

        if (!found){
            return false;
        }

        uint8_t sAlgo1 = (dcOption.info & 0x00FF0000) >> 16;
        uint8_t sAlgo2 = (dcOption.info & 0x0000FF00) >> 8;
        uint8_t sAlgo3 = (dcOption.info & 0x000000FF);

        return (sAlgo1 == algoCode || sAlgo2 == algoCode || sAlgo3 == algoCode);
    }

    void optionsOp(int connectionID, Option *options, uint8_t optLen){
        Option optionsSession[optLen];
        uint8_t optLenSession = 0;
        uint8_t currentSlot = 0;

        if (connections[connectionID].connectionPhase == CONTEXT_SYN_SENT || connections[connectionID].connectionPhase == CONTEXT_SYN_ACK_SENT){
            memcpy((void*)connections[connectionID].optionsUsed, (void*)options, 4 * optLen);
            connections[connectionID].optLenUsed = optLen;
            if (isNACKUsed(connections[connectionID].optionsUsed, connections[connectionID].optLenUsed)){
                connections[connectionID].NACKUsed = true;
            }
            uint8_t compAlgo =  isCompressionUsed(connections[connectionID].optionsUsed, connections[connectionID].optLenUsed);
            if (compAlgo != 0){
                connections[connectionID].dataCompressionUsed = true;
                connections[connectionID].dataCompressionAlgo = compAlgo;
                std::cerr << "DC is used with algo: " << (uint32_t)connections[connectionID].dataCompressionAlgo << std::endl;
            }
        }
        else if (connections[connectionID].connectionPhase == CONTEXT_SYN_EXPECTED){
            memcpy((void*)connections[connectionID].optionsUsed, (void*)options, 4 * optLen);
            connections[connectionID].optLenUsed = optLen;
            for (int i = 0; i < connections[connectionID].optLenUsed; i++) {
                for (int j = 0; j < optLen; j++) {
                    if ((connections[connectionID].optionsUsed + i)->type == (options + j)->type){
                        if ((options + i)->type == OPTION_NACK){
                            optionsSession[currentSlot].type = OPTION_NACK;
                            optionsSession[currentSlot].info = 0;
                            optionsSession[currentSlot].length = 0;
                            optLenSession++;
                        }
                        else if ((connections[connectionID].optionsUsed + i)->type == OPTION_DATA_COMPRESSION){
                            optionsSession[currentSlot].type = OPTION_DATA_COMPRESSION;
                            optionsSession[currentSlot].length = 8;
                            uint8_t cAlgo1 = ((options + j)->info & 0x00FF0000) >> 16;
                            uint8_t cAlgo2 = ((options + j)->info & 0x0000FF00) >> 8;
                            uint8_t cAlgo3 = ((options + j)->info & 0x000000FF);
                            if (supportsCompressionAlgo(connectionID, cAlgo1)){
                                optionsSession[currentSlot].info = cAlgo1 << 16;
                                optLenSession++;
                                std::cerr << "DC can be used with:" << (uint32_t)cAlgo1 << std::endl;
                            }
                            else if (supportsCompressionAlgo(connectionID, cAlgo2)){
                                optionsSession[currentSlot].info = cAlgo2 << 16;
                                optLenSession++;
                                std::cerr << "DC can be used with:" << (uint32_t)cAlgo2 << std::endl;
                            }
                            else if (supportsCompressionAlgo(connectionID, cAlgo3)){
                                optionsSession[currentSlot].info = cAlgo3 << 16;
                                optLenSession++;
                                std::cerr << "DC can be used with:" << (uint32_t)cAlgo3 << std::endl;
                            }

                        }
                    }
                }

            }
            memcpy((void*)connections[connectionID].optionsUsed, (void*)optionsSession, 4 * optLenSession);
            connections[connectionID].optLenUsed = optLenSession;
        }
    }
    
    void updateGlobalVariables(int connectionID, Header *headerPtr, int actualLen, bool updateRcvWnd){
        if (connections[connectionID].connectionPhase == CONTEXT_SYN_SENT || connections[connectionID].connectionPhase == CONTEXT_SYN_EXPECTED){
            if (headerPtr->version != 3){
                connections[connectionID].versionUsed = 0;
            }
        }
        //&& connections[connectionID].connectionPhase != CONTEXT_SYN_SENT
        if (connections[connectionID].connectionPhase != CONTEXT_SYN_EXPECTED){
            updateAckNr(connectionID, headerPtr->ackNr);
        }
        if (headerPtr->seqNr == connections[connectionID].seqExpected || connections[connectionID].connectionPhase == CONTEXT_SYN_EXPECTED || connections[connectionID].connectionPhase == CONTEXT_SYN_SENT || (connections[connectionID].NACKUsed)){
            connections[connectionID].seqExpected = calcNextSeqNr(headerPtr->seqNr);
        }
        if (headerPtr->seqNr == calcNextSeqNr(connections[connectionID].seqToack) || connections[connectionID].connectionPhase == CONTEXT_SYN_EXPECTED || connections[connectionID].connectionPhase == CONTEXT_SYN_SENT){
            connections[connectionID].seqToack = headerPtr->seqNr;
        }
        if (connections[connectionID].connectionPhase == CONTEXT_SYN_SENT || connections[connectionID].connectionPhase == CONTEXT_SYN_ACK_SENT){
            connections[connectionID].seqLastRead = headerPtr->seqNr;
        }
        updateSendWndSize(connectionID, headerPtr->wndSize);
        if (updateRcvWnd){
            updateRcvWndSize(connectionID, actualLen - 12, RCV_WND_SUBTRACT_MODE);
        }
    }

    void retransOp(int connectionID, Header* headerPtr, uint8_t optLen, Option* nackOpt, uint32_t seqExp){
    
        if(connections[connectionID].NACKUsed == true){
            //If we receive a NACK packet with bit map, we retransmit
            if((connections[connectionID].nackPhase == 0) && (connections[connectionID].connectionPhase == CONTEXT_DATA_TRANS)){
                for (int j = 0; j < optLen; j++) {
                    //If we receive a NACK packet, we retransmit
                    if ((headerPtr->options + j)->type == OPTION_NACK) {

                        //Gets the bit map from options and ignores the first 8 bits
                        uint32_t bit_map = (headerPtr->options + j)->info;
                        bit_map = bit_map & 0b00000000111111111111111111111111;
                        nack_retransmit(connectionID, bit_map);
                    }
                }
            }
            //If Nack is used, we sent a NACK and we receive retransmit packet
            if(connections[connectionID].nackPhase == 3){
                missing_seqs_received(connectionID, headerPtr->seqNr);
            }
            //If Nack is used and we receive an unexpected packet
            if((seqExp < headerPtr->seqNr) && (connections[connectionID].connectionPhase == CONTEXT_DATA_TRANS)) {

                //If we receive something out of order, we check if it's a retransmit from previous NACK,
                //if we haven't received everything, we can't do anything
                if (!(missing_seqs_received(connectionID, headerPtr->seqNr)) && (connections[connectionID].nackPhase == 3)) {
                } 
                else{
                    //Missing packet has been detected
                    connections[connectionID].nackPhase = 1;

                    std::cout << "Missing packet detected, NACK activated!\n";

                    uint32_t missing_seqs = headerPtr->seqNr - seqExp;

                    //If our vector is full, we must send NACK NOW
                    if(connections[connectionID].nCounter == 24) {
                        char empty_array[0];
                        Option *nackOpt;
                        uint8_t optLen = 0;
                        nackOpt = nack_list(connectionID);
                        optLen++;
                        stpSend(connectionID, empty_array, 0, 0, nackOpt, optLen, connections[connectionID].seqNext);
                        connections[connectionID].nackPhase = 2;
                    }
                    //If our vector isn't full, so we can keep adding more missing seqs
                    else{
                        //We need to be able to save multiple seqs, so we use this to insert every missing seq
                        //For example if we expected 10 but receive 15, we save 10-14
                        for(int i = 0; i < missing_seqs; i++){
                            //Inserts one by one
                            if(insert_received_seq(connectionID, seqExp + i)){
                            }
                            else{
                                //This quits the loop
                                i = missing_seqs;
                            }
                        }
                    }
                }
            }
        }
        if((connections[connectionID].NACKUsed == false) && (connections[connectionID].connectionPhase == CONTEXT_DATA_TRANS)){
            retransmitAll(connectionID);
        }
    }

    void globalTimerCheckAndTerminate(int connectionID){
        if (connections[connectionID].globalTimer == FINISHED_TIMER){
            std::cerr << "Terminating Connection... Reason: Global Timeout" << std::endl;
            terminateConnection(connectionID);
        }
    }

    void heartbeatCheckAndSend(int connectionID){
        if (connections[connectionID].heartbeatTimer == FINISHED_TIMER){
            heartBeat(connectionID);
        }
    }

//This method reads from a UDP packet, analyse its header and write the payload into the receive buffer.
//Return false on failure, true on success
    int stpReceive(int connectionID){
        std::lock_guard<std::mutex> guard(mu);
        int actualLen;
        int success;
        uint8_t optLen;
        Option *options;
        Option* nackOpt;
        auto *headerPtr = new Header(0,0,0,0, connections[connectionID].versionUsed,0,options);

        //Try to read a udp packet
        if (connections[connectionID].connectionPhase == CONTEXT_SYN_EXPECTED){
            struct sockaddr_storage addr;
            *connections[connectionID].dstLenPtr = sizeof(addr);
            actualLen = recvfrom(connections[connectionID].sockfd, connections[connectionID].workingBuffer, 65535, 0, connections[connectionID].dst, connections[connectionID].dstLenPtr);
            connections[connectionID].dstLen = *connections[connectionID].dstLenPtr;
            if(actualLen > 12){
                actualLen = 12;
            }
        }
        else{
            actualLen = recvfrom(connections[connectionID].sockfd, connections[connectionID].workingBuffer, 65535, 0, nullptr, connections[connectionID].dstLenPtr);
            globalTimerCheckAndTerminate(connectionID);
            if (connections[connectionID].connectionTerminated){
                return 0;
            }
            heartbeatCheckAndSend(connectionID);
        }

        //std::cerr << "UDP recved: " << actualLen <<std::endl;

        if (actualLen < 12){
            return -1;
        }

        resetTimer(GLOBAL_TIMEOUT_MILLISEC, connectionID, GLOBAL_TIMER);

        //try to read the STP header
        success = readHeader(headerPtr, connections[connectionID].workingBuffer, connections[connectionID].connectionPhase, connectionID);
        //Flag combination error: 1
        //Wrong seq num: 2
        //RST bit set: 3
        //Unexpected SYN: 4
        //Success: 0

        if (success == 1){
            return -1;
        }
        if (success == 2 && connections[connectionID].connectionPhase != CONTEXT_SYN_EXPECTED){
            return -1;
        }
        if(success == 3) {
            /*if (connections[connectionID].remoteIP == nullptr){
                listen(connections[connectionID].localPort, connections[connectionID].protocol,
                        connections[connectionID].sendBufferMaxSize, connections[connectionID].receiveBufferMaxSize,
                        connections[connectionID].optionsUsed, connections[connectionID].optLenUsed);
            }
            else{
                connect(connections[connectionID].remoteIP, connections[connectionID].remotePort, connections[connectionID].protocol,
                        connections[connectionID].sendBufferMaxSize, connections[connectionID].receiveBufferMaxSize,
                        connections[connectionID].optionsUsed, connections[connectionID].optLenUsed);
            }*/
            std::cerr << "RST bit detected, terminating connection" << std::endl;
            terminateConnection(connectionID);
            return -1;
        }
        if (success == 4){
            reset(connectionID);
            return -1;
        }

        optLen = headerPtr->optLen;

        int start_of_payload = 12 + 4*optLen;
        uint32_t payload_length = actualLen-start_of_payload;

        //check if compression ist enabled
        if(connections[connectionID].dataCompressionUsed && payload_length > 0){
			int decompressed_buf_len = payload_length*4;
			char* decompressed_payload;
			uint32_t decomp_amt = 0;

			if(connections[connectionID].dataCompressionAlgo == 0x2){
				decompressed_payload = new char[decompressed_buf_len];
				lzw decomp(10);
				decomp_amt = decomp.decompress(connections[connectionID].workingBuffer + start_of_payload,
						payload_length, decompressed_payload, decompressed_buf_len);
				actualLen = decomp_amt + start_of_payload;
			} else if (connections[connectionID].dataCompressionAlgo == 0x5){
				huffman decomp;
				decompressed_payload = decomp.decompress(connections[connectionID].workingBuffer + start_of_payload,
						decomp_amt);
				actualLen = decomp_amt + start_of_payload;
			}
			memcpy(connections[connectionID].workingBuffer+start_of_payload, decompressed_payload, decomp_amt);
			delete[] decompressed_payload;
        }

        bool updateRcvWnd = false;

        //try to write in our Buffer
        if(lookUpSeqInRcvBuffer(connectionID,headerPtr->seqNr) > 65535 && validateSeqNr(connectionID,headerPtr->seqNr)){
            //std::cerr << "New Packet, writing in rcvBuffer." << std::endl;
            updateRcvWnd = true;
            success = writeInRcvRingBuffer(connectionID, connections[connectionID].workingBuffer + 12 + 4*optLen, actualLen - 12 - 4*optLen, headerPtr->seqNr);
            if (!success){
                std::cerr << "Recv Buffer Full" << std::endl;
                return -1;
            }
        }


        //Needed for NACK
        uint32_t old_seq_expected = connections[connectionID].seqExpected;

        //update send buffer, send window
        updateGlobalVariables(connectionID, headerPtr, actualLen, updateRcvWnd);

        //Operations with flags
        flagOp(connectionID, headerPtr->flags);

        if(connections[connectionID].connectionTerminated){
            return 0;
        }

        //Operations with Options
        if (connections[connectionID].versionUsed == 3){
            if (connections[connectionID].connectionPhase == CONTEXT_SYN_SENT || connections[connectionID].connectionPhase == CONTEXT_SYN_EXPECTED
                || connections[connectionID].connectionPhase == CONTEXT_SYN_ACK_SENT){
                optionsOp(connectionID, headerPtr->options, headerPtr->optLen);
            }
        }

        //Handles retransmission depending on if NACK is used
        if(connections[connectionID].connectionPhase != CONTEXT_SYN_SENT && connections[connectionID].connectionPhase != CONTEXT_SYN_EXPECTED){
            retransOp(connectionID, headerPtr, optLen, nackOpt, old_seq_expected);
        }

        //std::cerr << "Recieved seq:" << headerPtr->seqNr << " with size: " << actualLen << std::endl;
        //std::cerr << "WorkingBuffer[12]: " << (int)connections[connectionID].workingBuffer[12] << "\n";
        return actualLen;
    }

//This method writes a STP packet and send it as a UDP packet, backup the packet into the send buffer.
//Return false on failure, true on success
    bool stpSend(int connectionID, void* buf, int len, uint8_t flags, Option *options, uint8_t optLen, uint32_t seqNr){
        int status = -2;
        bool success;

        uint8_t headerLen = (3 + optLen)*4;
        int msgLen = headerLen + len;


        //reserve memory for our STP packet and reserve packet
        auto *msg = new char[msgLen];
        auto *rsvPayload = new char[len];

        //Write Header
        //std::cerr << "rcvWnd: " << connections[connectionID].rcvWnd << std::endl;
        Header *header = new Header(seqNr, connections[connectionID].seqToack, connections[connectionID].rcvWnd, flags,connections[connectionID].versionUsed, optLen, options);
        success = writeHeader(*header, msg);
        delete(header);

        if (!success){
            std::cerr << "writeHeader failed" << std::endl;
            sleep(5);
            delete[] (msg);
            return false;
        }

        //Concatenate header with message
        if (len > 0){
            memcpy(rsvPayload, buf, len);
            memcpy(msg + headerLen, buf, len);
        }

        //check if compression ist enabled
        if(connections[connectionID].dataCompressionUsed &&
        		connections[connectionID].dataCompressionAlgo == 0x2 && len > 0){
        	lzw comp(len);
        	uint32_t comp_buf_len = len*2;
        	char* comp_buf = new char[comp_buf_len];
        	int comp_amt = comp.compress(rsvPayload, len, comp_buf, comp_buf_len);

        	//Concatenate header with message
        	memcpy(msg + headerLen, comp_buf, comp_amt);
        	msgLen = headerLen + comp_amt;
        	
        	delete[] comp_buf;
        } else if (connections[connectionID].dataCompressionUsed &&
        		connections[connectionID].dataCompressionAlgo == 0x5 && len > 0) {
        	//huffman compression
        	huffman huf;
        	uint32_t comp_buf_len = 0;
        	char* comp_buf = huf.compress(rsvPayload, len, comp_buf_len);
        	memcpy(msg + headerLen, comp_buf, comp_buf_len);
        	delete[] comp_buf;
        	msgLen = headerLen + comp_buf_len;
    	}else{
			//Concatenate header with message
			if (len > 0){
				memcpy(msg + headerLen, buf, len);
			}
        }


        //backup packet in our send buffer if there is no packet with given seqNr in Buffer
        uint32_t slotNr = lookUpSeqInSendBuffer(connectionID, seqNr);
        if (slotNr > 65535){
            success = writeInSendBuffer(connectionID, seqNr, flags, options, optLen, rsvPayload, len);
            if (!success){
                std::cerr << "Send buffer full" << std::endl;
                sleep(5);
                delete[] (msg);
                return false;
            }
        }

        if(connections[connectionID].sendWnd >= msgLen && calcNextSeqNr(seqNr) != connections[connectionID].seqUnack &&
                (!connections[connectionID].congestionControlUsed || congestionControl(connectionID))){
            status = sendto(connections[connectionID].sockfd, msg, msgLen, 0, connections[connectionID].dst, connections[connectionID].dstLen);
        }

        //std::cerr << "msg[12]: " << (int)msg[12] << "\n";
        if (status == -1){
            std::cerr << "sendto failed" << std::endl;
            delete[] (msg);
            return false;
        }

        if (status == -2){
            std::cerr << "CC or sendwnd or Seq" << std::endl;
            std::cerr << "sendWnd: " << connections[connectionID].sendWnd << std::endl;
            std::cerr << "SeqNext: " << connections[connectionID].seqNext << std::endl;
            std::cerr << "SeqUnack: " << connections[connectionID].seqUnack << std::endl;
            delete[] (msg);
            return false;
        }

        //std::cerr << "UDP sent: " << status << std::endl;

        //Update seqNext If it was a new packet
        if (seqNr == connections[connectionID].seqNext){
            //std::cerr << "New Packet, updating sendWnd" << std::endl;
            updateSendWndSize(connectionID, connections[connectionID].sendWnd - msgLen);
            connections[connectionID].seqNext = calcSequenceNumber(NEXT_SEQ_MODE,connections[connectionID].seqNext);
        }

        delete[] (msg);

        //std::cerr << "sent seq: " << seqNr << "\n";// << std::endl;

        return true;
    }

    bool readFromSendRingBuffer(int connectionID, uint32_t seqNr, BufferPacket* bufPacketPtr){
        uint32_t slotNr = lookUpSeqInSendBuffer(connectionID, seqNr);

        //If seqNr not found, return false
        if (slotNr > 65535){
            return false;
        }

        BufferPacket packet = *connections[connectionID].sendRingBuffer[slotNr];

        bufPacketPtr->seqNr = packet.seqNr;
        bufPacketPtr->flags = packet.flags;
        bufPacketPtr->options = packet.options;
        bufPacketPtr->optLen = packet.optLen;
        bufPacketPtr->payloadPtr = packet.payloadPtr;
        bufPacketPtr->length = packet.length;

        return true;
    }

//Retransmit a packet with given sequence number.
    bool retransmit(int connectionID, uint32_t seqNr){
        //std::cerr << "retransmitting seq: " << seqNr << std::endl;
        uint32_t slotNr = lookUpSeqInSendBuffer(connectionID, seqNr);
        bool success;

        //If seqNr not found, return false
        if (slotNr > 65535){
            return false;
        }

        BufferPacket packet = *connections[connectionID].sendRingBuffer[slotNr];

        //create an option array from pointer and optLen
        Option *options = new Option[packet.optLen];
        for (int i = 0; i < packet.optLen; i++) {
            *(options + i) = *(packet.options + i);
        }

        success = stpSend(connectionID, packet.payloadPtr, packet.length, packet.flags, options, packet.optLen, seqNr);

        delete[](options);

        return success;
    }

// retransmit all packets that are sent but not acknowledged
// No return value
    void retransmitAll(int connectionID){
        //std::cerr << "retransmitting all" << std::endl;
        uint32_t seqNr = connections[connectionID].seqUnack;
        while (seqNr != connections[connectionID].seqNext){
            retransmit(connectionID, seqNr);
            seqNr = calcNextSeqNr(seqNr);
        }
    }
}

