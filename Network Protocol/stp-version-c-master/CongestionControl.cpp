// SLOW START: Everytime a packet is send, cCounter gets bigger by 1.
//              After the cTimer is restarted, cWnd += cCounter
//              The maximum increase of cWnd is cWnd*=2
//              -> If no packets are send, cWnd doesnt increase
// Congestion Avoidance: Everytime a packet is send, cCounter gets bigger by 1.
//              After cTimer is restarted, cWnd++
//              Increase of cWnd is linear
//              -> Even if no packets are send, cWnd increases linear

#include "CongestionControl.h"
#include "Timers.h"

namespace stp{
extern Connection connections[MAX_NUMBER_OF_CONNECTIONS];

//Calculates if there are unack packets. Returns true if there are and false if not.
bool congestionDetect(int connectionID){
    //If more than 2 Pakets are missing a Congestion is Detected.
    if (connections[connectionID].seqNext - connections[connectionID].seqUnack > 2){
        return true;
    } else {
        return false;
    }
}

//Calculates congestion mode. Returns 1 for Slow Start, 2 for Congstion Avoidance.
int calcCongestionModus(int connectionID){
    int modus;
    if(connections[connectionID].cWnd < connections[connectionID].cThreshold){
        modus = 1;
    } else {
        modus = 2;
    }
    return modus;
}

//Update cWnd and cThreshold after failure event in Slow Start modus. (cThreshold = cThreshold/2)
void decreaseValuesSlowStart(int connectionID){
    connections[connectionID].cWnd = MIN_CONGESTION_WINDOW;
    connections[connectionID].cCounter = 0;

    if (connections[connectionID].cThreshold > 1){
        connections[connectionID].cThreshold = connections[connectionID].cThreshold / 2;
    }
}

//Update cWnd and cThreshold after failure event in Congestion Avoidance modus. (cThreshold = cWnd/2)
void decreaseValuesCAvoidance(int connectionID){
    connections[connectionID].cWnd = MIN_CONGESTION_WINDOW;
    connections[connectionID].cCounter = 0;

    if (connections[connectionID].cThreshold > 1){
        connections[connectionID].cThreshold = connections[connectionID].cThreshold = connections[connectionID].cWnd / 2;
    }
}

//Slow Start algorithm. Returns true if packet can be send, false if not.
bool slowStart(int connectionID){
    //Checking here for cTimer.
    if (connections[connectionID].cTimer == FINISHED_TIMER){
        
        //Check for Unack packets
        if (congestionDetect(connectionID)){
            //If yes call decreaseValuesSlowStart function and start new timer and dont send a packet.
            decreaseValuesSlowStart(connectionID);
            timer(CONGESTION_TIME, connectionID, CONGESTION_TIMER);
            return false;
        } else {
            //If no then update cWnd+=cWndAdd and cCounter=1 and start new timer and send packet.
            connections[connectionID].cWnd += connections[connectionID].cCounter;
            connections[connectionID].cCounter = 1;
            timer(CONGESTION_TIME, connectionID, CONGESTION_TIMER);
            return true;
        }
    

    } else if (connections[connectionID].cTimer == RUNNING_TIMER){
        //Only sending the packet if cCounter is not bigger than cWnd
        if (connections[connectionID].cCounter < connections[connectionID].cWnd){
            connections[connectionID].cCounter++;
            return true;
        } else {
            return false;
        }
    }
}

//Congestion Avoidance algorithm. Returns true if packet can be send, false if not.
bool congestionAvoidance(int connectionID){
    //Checking here for cTimer.
    if (connections[connectionID].cTimer == FINISHED_TIMER){
        //Check for Unack packets.
        if (congestionDetect(connectionID)){
            //If yes call decreaseValuesCAvoidance function and start new timer and dont send a packet.
            decreaseValuesCAvoidance(connectionID);
            timer(CONGESTION_TIME, connectionID, CONGESTION_TIMER);
            return false;
        } else {
            //If no then update cWnd++ and cCounter=1 and start new timer and send packet.
            connections[connectionID].cWnd++;
            connections[connectionID].cCounter = 1;
            timer(CONGESTION_TIME, connectionID, CONGESTION_TIMER);
            return true;
        }
    
    
    } else if (connections[connectionID].cTimer == RUNNING_TIMER){
        //Only sending the packet if cCounter is not bigger than cWnd
        if (connections[connectionID].cCounter < connections[connectionID].cWnd){
            connections[connectionID].cCounter++;
            return true;
        } else {
            return false;
        }
    }
}

//Public method that is called in stp_send. It determines if a packet can be send or not.
//Returns true if packet can be send, false if not.
bool congestionControl(int connectionID){

    //If Connection Phase is not DATA TRANSFER send all packets
    if (connections[connectionID].connectionPhase != CONTEXT_DATA_TRANS){
        return true;
    }

    int modus = calcCongestionModus(connectionID);

    //Start a cTimer at the beginning of a Connection phase
    if (connections[connectionID].cTimer == NOT_EXISTING_TIMER){
        timer(CONGESTION_TIME, connectionID, CONGESTION_TIMER);
    }

    //Procedure for Slow Start modus
    if (modus == SLOW_START){
        return slowStart(connectionID);
    }

    //Procedure for Congestion Avoidance modus
    if (modus == CONGESTION_AVOIDANCE){
        return congestionAvoidance(connectionID);
    }
}
}
