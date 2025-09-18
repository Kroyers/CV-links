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
#include "Option.h"
#include "Connection.h"
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include "Interface.h"

int main(int argc, char *argv[]) {
    if (argc <= 3){
        std::cerr << "Not enough arguments\n";
        std::cerr << std::endl;
        return 0;
    }

    bool success = false;
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

    int connectionIDC = stp::connect(argv[1], argv[2], atoi(argv[3]), 20000, 20000, options, optLenC);

    if (connectionIDC < 0){
        std::cerr << "Failed to connect\n";
        std::cerr << std::endl;
        return -1;
    }

    std::string msg;
    std::cout << "Please enter a message: ";
    std::cin >> msg;
    std::cout << std::endl;

    int sendLen = msg.length();
    int temp = 0;
    char *buf = new char[sendLen];
    for (int i = 0; i < sendLen; i++){
        *(buf + i) = msg[i];
    }

    while (!success){
        success = stp::send(connectionIDC, buf, sendLen);
    }
    std::cerr << "Sent message with length: " << sendLen << std::endl;

    while (length < sendLen){
        temp = stp::receive(connectionIDC, buf + length, sendLen - length);
        if (temp > 0){
            length += temp;
        }
    }
    std::cerr << "Received message with length: " << length << std::endl;

    for (int i = 0; i < length; i++) {
        std::cerr << *(buf + i);
    }

    std::cerr << "\nclosing connection" << std::endl;
    stp::stpClose(connectionIDC);
    return 0;
}