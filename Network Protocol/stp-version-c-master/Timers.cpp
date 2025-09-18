#include "Connection.h"
#include <iostream>
#include <thread>
#include <chrono>

//Variable needs to be implemented in Connection.h

namespace stp{

    extern Connection connections[MAX_NUMBER_OF_CONNECTIONS];

    //Wait function.
    void longWait(int milliseconds){
        std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
    }

    //Wait function. Returns false if timer was reset.
    bool shortWait(int connectionID, int timerSelect){
        //Wait will be stopped if reset[]Timer is set to true.
        for(int i=0; i<20; i++){
            switch(timerSelect){
                case HEARTBEAT_TIMER:
                    if(connections[connectionID].resetHeartbeatTimer){
                        connections[connectionID].resetHeartbeatTimer = false;
                        return false;
                    } else {
                        std::this_thread::sleep_for(std::chrono::milliseconds(connections[connectionID].heartbeatTime/20));
                    }
                    break;
                case GLOBAL_TIMER:
                    if(connections[connectionID].resetGlobalTimer){
                        connections[connectionID].resetGlobalTimer = false;
                        return false;
                    } else {
                        std::this_thread::sleep_for(std::chrono::milliseconds(connections[connectionID].globalTime/20));
                    }
                    break;
                case RETRANS_TIMER:
                    if(connections[connectionID].resetRetransTimer){
                        connections[connectionID].resetRetransTimer = false;
                        return false;
                    } else {
                        std::this_thread::sleep_for(std::chrono::milliseconds(connections[connectionID].retransTime/20));
                    }
                    break;
            }
        }
        return true;
    }


    //Individual cTimer.
    void longWaitThread(int milliseconds, int connectionID, int timerSelect){
        switch(timerSelect){
            case CONGESTION_TIMER:
                connections[connectionID].cTimer = RUNNING_TIMER;
                break;
        }
        longWait(milliseconds);
        switch(timerSelect){
            case CONGESTION_TIMER:
                connections[connectionID].cTimer = FINISHED_TIMER;
                break;
        }
    }

    //Individual hTimer.
    void shortWaitThread(int milliseconds, int connectionID, int timerSelect){
        switch(timerSelect){
            case HEARTBEAT_TIMER:
                connections[connectionID].heartbeatTimer = RUNNING_TIMER;
                break;
            case GLOBAL_TIMER:
                connections[connectionID].globalTimer = RUNNING_TIMER;
                break;
            case RETRANS_TIMER:
                connections[connectionID].retransTimer = RUNNING_TIMER;
                break;
        }

        bool finished;
        do {
            finished = shortWait(connectionID, timerSelect);
        } while (finished == false);

        switch(timerSelect){
                case HEARTBEAT_TIMER:
                    if(connections[connectionID].heartbeatTimer == RUNNING_TIMER){
                        connections[connectionID].heartbeatTimer = FINISHED_TIMER;
                    }
                    break;
                case GLOBAL_TIMER:
                    if(connections[connectionID].globalTimer == RUNNING_TIMER) {
                        connections[connectionID].globalTimer = FINISHED_TIMER;
                    }
                    break;
                case RETRANS_TIMER:
                    if(connections[connectionID].retransTimer == RUNNING_TIMER) {
                        connections[connectionID].retransTimer = FINISHED_TIMER;
                    }
                    break;
        }
    }

    //Public function to restart timer. Returns false if Timer needs to be initialised.
    bool resetTimer(int milliseconds, int connectionID, int timerSelect){
        switch(timerSelect){
            case HEARTBEAT_TIMER:
                if(connections[connectionID].heartbeatTimer == FINISHED_TIMER){
                    return false;
                } else {
                    connections[connectionID].resetHeartbeatTimer = true;
                    connections[connectionID].heartbeatTime = milliseconds;
                    return true;
                }
            case GLOBAL_TIMER:
                if(connections[connectionID].globalTimer == FINISHED_TIMER){
                    return false;
                } else {
                    connections[connectionID].resetGlobalTimer = true;
                    connections[connectionID].globalTime = milliseconds;
                    return true;
                }
            case RETRANS_TIMER:
                if(connections[connectionID].retransTimer == FINISHED_TIMER){
                    return false;
                } else {
                    connections[connectionID].resetRetransTimer = true;
                    connections[connectionID].retransTime = milliseconds;
                    return true;
                }
        }
        return false;
    }


    //Public function that will start a timer.
    void timer(int milliseconds, int connectionID, int timerSelect){
        
        if (timerSelect == CONGESTION_TIMER){
            std::thread thread(longWaitThread, milliseconds, connectionID, timerSelect);
            thread.detach();
        }

        if (timerSelect == HEARTBEAT_TIMER){
            if(connections[connectionID].heartbeatTimer == RUNNING_TIMER) {
                connections[connectionID].resetHeartbeatTimer = true;
            }
            std::thread thread(shortWaitThread, milliseconds, connectionID, timerSelect);
            thread.detach();
        }

        if (timerSelect == GLOBAL_TIMER){
            if(connections[connectionID].globalTimer == RUNNING_TIMER) {
                connections[connectionID].resetGlobalTimer = true;
            }
            std::thread thread(shortWaitThread, milliseconds, connectionID, timerSelect);
            thread.detach();
        }

        if (timerSelect == RETRANS_TIMER){
            if(connections[connectionID].retransTimer == RUNNING_TIMER) {
                connections[connectionID].resetRetransTimer = true;
            }
            std::thread thread(shortWaitThread, milliseconds, connectionID, timerSelect);
            thread.detach();
        }
    }
}