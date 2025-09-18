#ifndef STP_PROTOCOL_TIMERS_H
#define STP_PROTOCOL_TIMERS_H

namespace stp{
    bool resetTimer(int milliseconds, int connectionID, int timerSelect);

    void timer(int milliseconds, int connectionID, int timerSelect);
}
#endif
