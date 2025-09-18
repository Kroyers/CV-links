# STP Version C

STP can be tested using Stp_Client_PingPong.cpp and Stp_Server_PingPong.cpp.

To compile client, this command should be used: make client
Then to run client, this command should be used: ./client ::1 9999 6
We tested it locally using IP: 127.0.0.1 and PORT: 9999 and 6 stands for IPv6.

To compile server, this command should be used: make server
Then to run server, this command should be used: ./server 9999 6
We tested it locally using IP: 127.0.0.1 and PORT: 9999 and 6 stands for IPv6.

STP Version C requires boost library installed to compile.
libboost-dev libboost-system-dev and libboost-thread-dev are required for the protocol.
Link libraries with following commands when compiling:
-lboost_system -lboost_thread

In order to use the functionality of STP version C, following header files needs to be included:
#include "Option.h"
#include "Connection.h"
#include "Interface.h"

When using functions from this protocol, use namespace stp

----------------------------------------------------------------------------------------------------------

Please Note that connecting to remote host using IPv6 with IPv4 address will cause undefined
Behavior and this should be avoided.

----------------------------------------------------------------------------------------------------------

Functions:
stp::connect(char* ip, char* port, uint8_t protocol, uint32_t sendWndInput, uint32_t rcvWndInput, 
Option options[],uint8_t optLenInput)

ip: The IP address you want to connect to.

port: The port of the remote host you want to connect to.

protocol: should be either 4 or 6. 4 stands for IPv4, 6 stands for IPv6. If the input is neither 4 or 6, IPv6 is used.

sendWndInput: Amount of send window in bytes. 
please note it is not strictly the amount of payload that can be buffered
since overhead is created when buffering a package.

rcvWndInput: Similar to sendWndInput.

options: The extensions that you wish to use in this connection. 

optLenInput: How many options do you have in the options array.

returns connectionID on successful call, negative value on failure.

------------------------------------------------------------------------------------------------------------------

stp::listen(char* localPort, uint8_t protocol, uint32_t sendWndInput, uint32_t rcvWndInput, 
Option* options, uint8_t optLen)

localPort: On which port would you like to listen.

protocol: should be either 4 or 6. 4 stands for IPv4, 6 stands for IPv6. If the input is neither 4 or 6, IPv6 is used.

sendWndInput: Amount of send window in bytes. 
please note it is not strictly the amount of payload that can be buffered
since overhead is created when buffering a package.

rcvWndInput: Similar to sendWndInput.

options: The extensions that you wish to use in this connection. 

optLen: How many options do you have in the options array.

returns connectionID on successful call, negative value on failure.

----------------------------------------------------------------------------------------------------------------------

stp::accept(int connectionID)

connectionID: connectionID returned in listen call.

returns connectionID on successful call, negative value on failure.

----------------------------------------------------------------------------------------------------------------------

stp::send(int connectionID, void* buf, int len)

connectionID: connectionID returned in listen call.

buf: pointer to the data that needs to be sent.

len: length of the data that needs to be sent.

returns true on success and false on failure.

----------------------------------------------------------------------------------------------------------------------

receive(int connectionID, void *buf, size_t len)

connectionID: connectionID returned in listen call.

buf: Data will be written into this pointer

len: length of data you want to receive.

returns length of data received in bytes on success and negative value on failure.

----------------------------------------------------------------------------------------------------------------------
Option: 
the fields are given below:
        uint8_t type              //The type of the option slot, must always be written into the first byte.
        uint32_t info             //The information we provide in this option slot.
        uint8_t length            //Number of bits used in this option slot. This is declared for easier padding.

type: What extension do you wish to use:
	Data compression   1
    Congestion control 2
    NACK               3

info: Informations about the extension:
	Congestion control   Set info to 0
    NACK                 Set info to 0
	Data compression     Set info to 0x00|0x02|0x00|0x00 for Huffman Encoding
									 0x00|0x05|0x00|0x00 for LZW Encoding
									 0x00|0x02|0x05|0x00 for both
	Please note that depending on what the remote host supports, only one Algorithm will be chosen.

length: set to 0.