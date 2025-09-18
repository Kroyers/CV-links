#include <boost/thread.hpp>
#include <boost/chrono.hpp>
#include <iostream>
#include "Interface.h"
#include "Connection.h"

#define MIN_CONGESTION_WINDOW 4
#define CONGESTION_TIME 2000
#define STARTING_THRESHOLD 32
#define SLOW_START 1
#define CONGESTION_AVOIDANCE 2

namespace stp{
    bool congestionControl(int connectionID);
}