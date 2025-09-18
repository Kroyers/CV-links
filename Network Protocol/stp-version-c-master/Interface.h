//
// Created by Qingyun on 02.12.2019.
//

#ifndef STP_PROTOCOL_MAIN_H
#define STP_PROTOCOL_MAIN_H

#include <stdint-gcc.h>
#include <sys/socket.h>
#include "Option.h"

namespace stp{
    #define CONTEXT_DATA_TRANS 0
    #define CONTEXT_SYN_SENT 1
    #define CONTEXT_SYN_RECEIVE 2
    #define CONTEXT_SYN_ACK_SENT 3
    #define CONTEXT_FIN_SENT 4
    #define CONTEXT_FIN_ACK_SENT 5
    #define CONTEXT_FIN_RECEIVED 6
    #define CONTEXT_SYN_EXPECTED 7

    #define SYN_FLAG 0b00001000
    #define SYN_ACK_FLAG 0b00001100
    #define ACK_FLAG 0b00000100
    #define FIN_FLAG 0b00000010
    #define FIN_ACK_FLAG 0b00000110
    #define RST_FLAG 0b00000001

    #define MAX_NUMBER_OF_CONNECTIONS 10

    #define MAX_SEQ_NR 16777216
    #define MAX_RINGBUFFER_SLOT 300

    #define SEND_BUFFER_MODE 0
    #define RECEIVE_BUFFER_MODE 1

    #define BUFFER_PACKET_OVERHEAD 2*sizeof(uint32_t)+sizeof(uint8_t)+sizeof(Option*)+sizeof(char*)

    #define INIT_SEQ_MODE 0
    #define NEXT_SEQ_MODE 1

    #define RCV_WND_ADD_MODE 0
    #define RCV_WND_SUBTRACT_MODE 1

    #define FINISHED_TIMER 0
    #define RUNNING_TIMER 1
    #define NOT_EXISTING_TIMER 2

    #define CONGESTION_TIMER 1
    #define HEARTBEAT_TIMER 2
    #define GLOBAL_TIMER 3
    #define RETRANS_TIMER 4

    #define GLOBAL_TIMEOUT_MILLISEC 10000
    #define HEARTBEAT_TIMEOUT_MILLISEC 5000

    const uint32_t MAX_BUFFER_SIZE = 4294967295;

    int send(int connectionID, void* buf, int len);

    int receive(int sockfd, void *buf, size_t len);

    void startTimers(int connectionID);

    void heartBeat(int connectionID);
}

#endif
