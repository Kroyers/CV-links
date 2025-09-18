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

    int connectionIDS = stp::listen( argv[1], atoi(argv[2]), 20000, 20000, options, optLenS);
    if (connectionIDS < 0){
        std::cerr << "Failed to listen\n";
        std::cerr << std::endl;
        return -1;
    }
    stp::accept(connectionIDS);

    int length = 0;
    int temp = 0;
    bool success = false;

    auto* buf = new char[1000];

    while (length <= 0){
        temp = stp::receive(connectionIDS, buf + length, 1000 - length);
        if (temp > 0){
            length += temp;
        }
    }
    std::cerr << "Received message with length: " << length << std::endl;
    for (int i = 0; i < length; i++) {
        std::cerr << *(buf + i);
    }
    std::cerr << std::endl;

    while (!success){
        success = stp::send(connectionIDS, buf, length);
    }
    std::cerr << "Sent message with length: " << length << std::endl;
    std::cerr << "Closing connection" << std::endl;
    stp::stpClose(connectionIDS);


    return 0;
}