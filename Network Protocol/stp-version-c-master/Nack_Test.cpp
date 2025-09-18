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
#include "Nack.h"
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include "Interface.h"
#include "Nack_Test.h"

namespace stp{
	extern Connection connections[MAX_NUMBER_OF_CONNECTIONS];

	void simulate_packet_loss(int connectionID, int offset){

		std::cout << "SeqNext before adjustment: " << connections[connectionID].seqNext << "\n";
		
		for(int i = connections[connectionID].seqNext; i < (connections[connectionID].seqNext + offset); i++){
			writeInSendBuffer(connectionID, i, 0, 0, 0, 0, 0);
			std::cout << "Dummy packet with " << i <<" has been written into send buffer" <<"\n";
		}
		
		connections[connectionID].seqNext = connections[connectionID].seqNext + offset;
		std::cout << "SeqNext after adjustment: " << connections[connectionID].seqNext << "\n";
	}
	/*
	bool insert_dummy_packet(int connectionID, uint32_t seqnr){
		return bool = writeInSendBuffer(connectionID, seqnr, 0, 0, 0, 0, 0);
	}
	*/
}