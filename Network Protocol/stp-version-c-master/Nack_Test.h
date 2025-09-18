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

namespace stp{

	void simulate_packet_loss(int connectionID, int offset);

	//void insert_dummy_packet(int connectionID, uint32_t seqnr);

}