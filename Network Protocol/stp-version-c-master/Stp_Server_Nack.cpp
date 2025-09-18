#include <iostream>
#include <sys/types.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <fcntl.h>
#include "Transmission.h"
#include "Connection.h"
#include <chrono>
#include <random>
#include <thread>
#include "Nack_Test.h"


int main(int argc, char *argv[]){
    if (argc <= 2){
        std::cerr << "Not enough arguments\n";
        std::cerr << std::endl;
        return 0;
    }

    stp::Option nackOption;
    nackOption.type = OPTION_NACK;
    nackOption.info = 0;
    nackOption.length = 0;

    stp::Option dcOption;
    dcOption.type = OPTION_DATA_COMPRESSION;
    dcOption.info = COMP_ALGO_HUFFMAN << 16;
    dcOption.length = 8;

    stp::Option ccOption;
    ccOption.type = OPTION_CONGESTION_CONTROL;
    ccOption.info = 0;
    ccOption.length = 0;

    stp::Option options[1] = {nackOption};
    uint8_t optLenS = 1;

    int connectionIDS = stp::listen( argv[1], 6, 3000, 3000, options, optLenS);
    stp::accept(connectionIDS);
    if (connectionIDS < 0){
        std::cerr << "Failed to listen\n";
        std::cerr << std::endl;
        return -1;
    }
    std::cerr << "Nack is used in this connection.\n";

    /*
    bool success = false;
    int length = 0;

    auto* buf = new uint32_t[1];
    uint32_t target = 0;
    auto* targetbuf = &target;

    target = atoi(argv[2]);

    std::cerr << "sending target: " << target << std::endl;

    while (!success){
        success = stp::send(connectionIDS, targetbuf, 4);
    }
    success = false;
    
    std::cerr << "Ping Pong now with target: " << target << std::endl;

    *(buf) = 0;
    std::cout << "Sending (s) and receiving (r): \n";
    while(*buf < target-1){
        while (length <= 0){
            length = stp::receive(connectionIDS, buf, 4);
        }
        printf("received: %i\n", *(buf));

        *buf += 1;

        while (!success){
            success = stp::send(connectionIDS, buf, 4);
            //printf("success: %i\n", success);
        }
        printf("sent: %i\n", *(buf));
        success = false;
        length = 0;
    }

    std::cerr << "\nPing Pong ends now" << std::endl;

    while (length <= 0){
        length = stp::receive(connectionIDS, buf, 1);
    }

    length = -1;

    while (length < 0){
        length = stp::receive(connectionIDS, buf, 1);
    }
    */


    auto* buf1 = new char[1];


    int len = 0;

    while (len <= 0){
        len = stp::receive(connectionIDS, buf1, 1);
        sleep(1);
    }

    std::cerr << "Packet 1 received.\n";


    len = 0;

    auto* buf2 = new char[1];

    while (len == 0){
        len = stp::receive(connectionIDS, buf2, 1);
    }

    std::cerr << "Packet 2 received, it should have activated NACK.\n";

    len = 0;

    bool success = false;

    auto* buf3 = new char[1];

    std::cerr << "Sending NACK packet now.\n";

    buf3[0] = 1;

    while (!success){
        success = stp::send(connectionIDS, buf3, 1);
        sleep(1);
    }

    success = false;

    len = 0;

    
    //std::cerr << "Receiving retransmissions: \n";
    /*
    for(int i = 0; i < 3; i++){
        int len1 = 0;
        std::cerr << "Receiving retransmissions of missing packet: "<< i <<"\n";
        while (len1 == 0){
            len1 = stp::receive(connectionIDS, buf, 1);
        }
        len1 = 0;
    }  
    */
    auto* bufr1 = new char[1];
    auto* bufr2 = new char[1];
    auto* bufr3 = new char[1];
    auto* bufr4 = new char[1];

    std::cerr << "Receiving retransmission of missing packet 1: \n";
    while (len == 0){
            len = stp::receive(connectionIDS, bufr1, 1);
    }
    len = 0;
    
    std::cerr << "Receiving retransmission of missing packet 2: \n";
    while (len == 0){
            len = stp::receive(connectionIDS, bufr1, 1);
    }
    len = 0;

    std::cerr << "Receiving retransmission of missing packet 3: \n";
    while (len == 0){
            len = stp::receive(connectionIDS, bufr1, 1);
    }
    len = 0;

    std::cerr << "Receiving retransmission of missing packet 4: \n";
    while (len == 0){
            len = stp::receive(connectionIDS, bufr1, 1);
    }
    len = 0;
    
    stp::stpClose(connectionIDS);
    std::cerr << std::endl;
    return 0;
}