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
#include <thread>
#include <chrono>

int main(int argc, char *argv[]) {
    if (argc <= 2){
        std::cerr << "Not enough arguments\n";
        std::cerr << std::endl;
        return 0;
    }

    bool err = false;
    stp::Option dcOption;
    dcOption.type = OPTION_DATA_COMPRESSION;
    dcOption.info = COMP_ALGO_LZW << 8;
    dcOption.info |= COMP_ALGO_HUFFMAN;
    dcOption.info = dcOption.info<<8;
    std::cerr << "info: " << dcOption.info << std::endl;
    dcOption.length = 16;
    stp::Option options[1] = {dcOption};
    int connectionIDC = stp::connect(argv[1], argv[2], 6, 21000, 21000, options, 1);
    if (connectionIDC < 0){
        std::cerr << "  FAILED TO CONNECT\n";
        std::cerr << std::endl;
        return -1;
    }    std::cerr << "  client connected"<< std::endl;
    char buf[16];
    int length = 0;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::cerr << "  client connected"<< std::endl;
    while (length < 16){
        int send = stp::receive(connectionIDC, buf+length, 16-length);
        if(send < 0){
            std::cerr << "  client receive error"<< std::endl;
            std::cerr << "  TEST FAILED"<< std::endl;
            return -1;
        }
        length += send;
    }
    for(int i =  0 ; i<16;i++)
    {
        std::cout<< buf[i];
    }
    std::cerr << "  closing client"<< std::endl;
    std::cerr << "  ";
    stp::stpClose(connectionIDC);
    return 0;



}