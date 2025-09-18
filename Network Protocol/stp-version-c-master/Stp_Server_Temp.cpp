#include "Connection.h"
#include <iostream>
#include <thread>
#include <vector>
#include <string>

int main(int argc, char* argv[]) {
    if(argc < 2) {
        std::cout << "Wrong number of arguments" << std::endl;
        return 0;
    }
    stp::Option dcOption;
    dcOption.type = OPTION_DATA_COMPRESSION;
    dcOption.info = COMP_ALGO_HUFFMAN << 16;
    dcOption.length = 8;
    stp::Option options[1] = {dcOption};
    int connectionIDS = stp::listen( argv[1], 6, 21000, 21000, options, 1);
    bool err = false;
    if (connectionIDS < 0){
        std::cerr << "  FAILED TO LISTEN\n";
        std::cerr << std::endl;
        return -1;
    }
    std::cerr << "  LISTEN\n";
    stp::accept(connectionIDS);
    std::cerr << "  server accepted"<< std::endl;
    char buf[16];
    buf[0]='L';
    buf[1]='Z';
    buf[2]='W';    
    buf[3]='A';
    buf[4]='B';
    buf[5]='C';
    buf[6]='L';
    buf[7]='Z';
    buf[8]='W';
    buf[9]='L'; 
    buf[10]='Z';
    buf[11]='W';
    buf[12]='L';
    buf[13]='Z';
    buf[14]='W';
    buf[15]='L'; 
    bool success = false;
    while (!success){
        std::cerr << "  server sending longmessage"<< std::endl;
        success = stp::send(connectionIDS, buf, 16);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(400));
    std::cerr << "  closing server"<< std::endl;
    stp::stpClose(connectionIDS);
    return 0;
}
