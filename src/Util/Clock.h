#ifndef CLOCK_H
#define CLOCK_H

#include <string>
#include <ctime>

class Clock
{
public:
    /// @return millsecs since system starts
    static uint64_t get_now_tick();
};

#endif // CLOCK_H
