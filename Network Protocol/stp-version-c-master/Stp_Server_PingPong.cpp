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

    stp::Option options[1] = {dcOption};
    uint8_t optLenS = 0;

    int connectionIDS = stp::listen( argv[1], 6, 3000, 3000, options, optLenS);
    stp::accept(connectionIDS);
    if (connectionIDS < 0){
        std::cerr << "Failed to listen\n";
        std::cerr << std::endl;
        return -1;
    }

    int success = -1;
    int length = 0;

    auto* buf = new uint32_t[1];
    uint32_t target = 0;
    auto* targetbuf = &target;

    target = atoi(argv[2]);

    std::cerr << "sending target: " << target << std::endl;

    while (success < 0){
        success = stp::send(connectionIDS, targetbuf, 4);
        if(success == -2){
            return -1;
        }
    }
    success = -1;

    std::cerr << "Ping Pong now with target: " << target << std::endl;

    *(buf) = 0;
    std::cout << "Sending (s) and receiving (r): ";
    while(*buf < target-1){
        while (length <= 0){
            length = stp::receive(connectionIDS, buf, 4);
        }
        printf("received: %i\n", *(buf));

        *buf += 1;

        while (success < 0){
            success = stp::send(connectionIDS, buf, 4);
            if(success == -2){
                return -2;
            }
            //printf("success: %i\n", success);
        }
        printf("sent: %i\n", *(buf));
        success = -1;
        length = 0;
    }

    std::cerr << "\nPing Pong ends now" << std::endl;

    while (length <= 0){
        length = stp::receive(connectionIDS, buf, 1);
        if(length == -2){
            return -1;
        }
    }

    length = -1;

    while (length < 0){
        length = stp::receive(connectionIDS, buf, 1);
        if(length == -2){
            return -1;
        }
    }

    stp::stpClose(connectionIDS);
    std::cerr << std::endl;
    return 0;
}