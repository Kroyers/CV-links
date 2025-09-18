#include "Nack.h"
//using namespace std; 

namespace stp{
extern Connection connections[MAX_NUMBER_OF_CONNECTIONS];

//Vector which is used to hold the SEQ of received packets
//vector<uint32_t> nack_vector{};

//Iterator which points at beginning of the nack_vector
//vector<uint32_t>::iterator ptr = nack_vector.begin();

//Counter to manage size of our list
//int counter = 0;

//Checkfs if the vector is empty
bool is_nack_empty(int connectionID){

    //Returns true on success
    if(connections[connectionID].nack_vector.empty()){ 
        return true;  
    } 
    return false; 
}

//Removes a seq from the vector
//Return value is success or failure
bool remove_received_seq(int connectionID, int position){
    //Checks if it's empty
    if(is_nack_empty(connectionID) == true){
        return false;
    }
    else{
        //Iterator to desired position
	    auto pos = connections[connectionID].nack_vector.begin() + position; 

        //Erases the element in the vector
        connections[connectionID].nack_vector.erase(pos);

        //Decrements the counter
        connections[connectionID].nCounter -= 1;
        /*
        //Testing output
        for(auto test = connections[connectionID].nack_vector.begin(); test  != connections[connectionID].nack_vector.end(); ++test ){
        }
        */
        return true;
    }
}

//Clears the whole NACK vector
void clear_nack(int connectionID){

    //Clears the vector
    connections[connectionID].nack_vector.clear();

    //Resets counter
    connections[connectionID].nCounter = 0;

}

//Inserts the SEQ of a received STP packet
//Returns false if it fails, true on success
bool insert_received_seq(int connectionID, uint32_t seqNr){

    //We can only keep tracks of 24 packets, so if our list isn't full
    if((connections[connectionID].nCounter < 23) && (connections[connectionID].nCounter >= 0)){
        
        std::cout << (uint32_t)seqNr <<" has been added as a missing packet."<< "\n";

        //Checks if it's empty
        if(is_nack_empty(connectionID) == true){

            //Iterator to the first position
	        auto next_pos = connections[connectionID].nack_vector.begin(); 

            //Inserts seqNr into the front
            connections[connectionID].nack_vector.insert(next_pos,seqNr);
            
            //Increases counter and returns true
            connections[connectionID].nCounter += 1;
            return true;
        }
        //If it's not empty
        else{
            //Iterator to the next free position, works with counter which is incremented after inserstions
	        auto next_pos = connections[connectionID].nack_vector.begin() + connections[connectionID].nCounter; 

            //Inserts the seqNr into the position
            connections[connectionID].nack_vector.insert(next_pos, seqNr);
            
            //Increases counter and returns true
            connections[connectionID].nCounter += 1;
            return true;
        }
    }
    //If our list is full
    else{
        return false;
    }
}

//Gets an element out of the vector
//Returns negative on failure, i.e. when vector is empty
uint32_t get_seq(int connectionID, int position){
    //If the vector is empty
    if(is_nack_empty(connectionID) == true){
        return -1;
    }
    else{
        //If the desired position it within the vector range
        if(position < (connections[connectionID].nack_vector.size())){

            //The value at the position
            uint32_t seq = connections[connectionID].nack_vector[position];
            return seq;
        }
        else{
            return -2;
        }
    }
}

//Returns an option object with the NACK option set and the bit map for NACKS
Option* nack_list(int connectionID){

    //Our return variable with static parts
    Option *nacks = new Option;
    nacks->type = OPTION_NACK;
    nacks->length = 24;

    nacks->info = 0x0;

    //First missing seq
    uint32_t trigger_point = get_seq(connectionID, 0);

    //The ACK of the next packet
    uint32_t ack = trigger_point - 1;

    int counter = 0;
    //We check our vector for stored SEQs, if there is a missing packet, we mark it in the bit map
    for(int i = 0; i < 23; i++){

        if((ack + i + 1) == (get_seq(connectionID, counter))){
            nacks->info |= 0x1;
            nacks->info = nacks->info << 1;
            counter += 1;
        }
        else{
            nacks->info = nacks->info << 1;
        }

    }  
    //We use this variable to ensure padding
    //int padding = 24 - connections[connectionID].nCounter;
     //nacks.info = nacks.info << padding;
     


    return nacks;
}

//This function decides which packets to retransmit when a NACK packet with a bit map has been received
void nack_retransmit(int connectionID, uint32_t bit_map){

    //According to the draft, we choose minimum
    uint32_t minimum = std::min((connections[connectionID].seqUnack + 24), connections[connectionID].seqNext);


    //Selects the first bit
    uint32_t selecton_bit = 0b00000000100000000000000000000000;
    
    uint32_t unack =  connections[connectionID].seqUnack;

    //For loop that goes through the bit map and decides which packets need to be retransmitted
    for(uint32_t retransmit_seq = unack; retransmit_seq < minimum; retransmit_seq++){
        //First bit in the bit map
        uint32_t bit = selecton_bit & bit_map;

        //If it's set to one
        if(bit == 0b00000000100000000000000000000000){

            //This means retransmit_seq must be retransmitted, it is incremented until the limit
            //For example we receive AckNr 100, then if 1st bit in bit map is set to 1, 
            //we must retransmit AckNr + 1, since AckNr was the last correctly received packet
            std :: cout << "Seq to be retransmitted is: " << (uint32_t)retransmit_seq<< " \n";
            retransmit(connectionID, retransmit_seq);

            //Shift bit map to left, so that the next bit is the left most
            bit_map = bit_map << 1;
        }
        else{
            //Else we dont retransmit it, but we still have to shift
            bit_map = bit_map << 1;
        }
    }
}

//Checks if all missing packets, that have been requested through NACK, have been received
bool missing_seqs_received(int connectionID, uint32_t seq){

    //Control variable
    //bool all_received = false;

    //Control variable
    int missing_count = 0;

    //We check our vector to see, if the missing packets have been received
    for(int i = 0; i < connections[connectionID].nCounter; i++){

        //We get a element of the vector
        uint32_t seq_in_vector = get_seq(connectionID, i);

        //We need this for the special case of first element
        if(i == 0){
            //If we did receive it
            if(seq_in_vector == seq){

            }
            else{
                missing_count += 1;
            }
        }
        else{
            //If the next element's value is only aa increment of 1, compared to the current element, there isn't a missing packet
            if((seq_in_vector - 1) == (get_seq(connectionID, (i - 1)))){
                //Here we do nothing

            }
            //If the next element's value is not an increment of 1, then there is a packet missing 
            else{
                //If we did receive the missing packet, it should be the same
                if(seq_in_vector == seq){

                }
                //It's missing
                else{
                    missing_count += 1;
                }
            }
        }
        //If all the requested missing packets are received, we clear our vector and set phase to normal operation
        if(missing_count == 0){
            connections[connectionID].nackPhase = 0;
            clear_nack(connectionID);
            return true;
        }
        else{
            return false;
        }
    }
    //If all the requested missing packets are received, we clear our vector and set phase to normal operation
    return false;
}
}
