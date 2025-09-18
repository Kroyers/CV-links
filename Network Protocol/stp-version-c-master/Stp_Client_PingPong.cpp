#include <iostream>
#include <sys/types.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <fcntl.h>
//#include "Transmission.h"
#include "Option.h"
#include "Connection.h"
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include "Interface.h"
#include <chrono>
#include <random>
#include <thread>

int main(int argc, char *argv[]) {
    if (argc <= 2){
        std::cerr << "Not enough arguments\n";
        std::cerr << std::endl;
        return 0;
    }

    int success = -1;
    int length = 0;

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

    stp::Option options[1] = {dcOption};
    uint8_t optLenC = 0;

    int connectionIDC = stp::connect(argv[1], argv[2], 6, 3000, 3000, options, optLenC);

    if (connectionIDC < 0){
        std::cerr << "Failed to connect\n";
        std::cerr << std::endl;
        return -1;
    }

    auto* buf = new uint32_t[1];
    uint32_t target = 0;
    auto* targetbuf = &target;

    while (length <= 3){
        length = stp::receive(connectionIDC, targetbuf, 4);
    }

    length = 0;

    std::cerr << "Ping Pong now with target: " << target << std::endl;

    *(buf) = 0;
    std::cout << "Sending (s) and receiving (r): ";

    while(*buf < target){

        while (success < 0){
            success = stp::send(connectionIDC, buf, 4);
            if(success == -2){
                return -1;
            }
        }
        printf("sent: %i\n", *(buf));

        while (length <= 0){
            length = stp::receive(connectionIDC, buf, 4);
        }
        printf("received: %i\n", *(buf));
        *buf += 1;

        success = -1;
        length = 0;
    }

    std::cerr << "\nPing Pong ends now" << std::endl;

    while (success < 0){
        success = stp::send(connectionIDC, buf, 1);
        if(success == -2){
            return -1;
        }
    }

    stp::stpClose(connectionIDC);
    std::cerr << std::endl;
    return 0;
}