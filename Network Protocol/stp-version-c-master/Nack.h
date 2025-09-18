#ifndef STP_PROTOCOL_NACK_H
#define STP_PROTOCOL_NACK_H

#include "Connection.h"
#include "Option.h"
#include "Interface.h"
#include "Transmission.h"
#include <vector> 
#include <iostream>
#include <iterator>
namespace stp{
//Function which should be called in stp_receive
//Inserts the SEQ of a received STP packet into our list
//Returns false if it fails, true on success
bool insert_received_seq(int connectionID, uint32_t seqNr);

//Returns an option object with the NACK option set and the bit map for NACKS, should be called when sending payload
Option* nack_list(int connectionID);

//Clears nack list for the connection, should be called when terminating a connection
void clear_nack(int connectionID);

//Upon receiving a NACK packet, this function processes the NACK bit map and decides which packets to retransmit
void nack_retransmit(int connectionID, uint32_t bit_map);

//Checks if we received all of requested missing packets
bool missing_seqs_received(int connectionID, uint32_t seq);

//Checks if nack vector is empty
bool is_nack_empty(int connectionID);

//For testing
uint32_t get_seq(int connectionID, int position);
bool remove_received_seq(int connectionID, int position);
bool is_nack_empty(int connectionID);
}
#endif //STP_PROTOCOL_NACK_H