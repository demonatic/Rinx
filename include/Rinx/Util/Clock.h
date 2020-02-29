#ifndef CLOCK_H
#define CLOCK_H

#include <string>
#include <ctime>

namespace Rinx {

class Clock
{
public:
    /// @return millsecs since system starts
    static uint64_t get_now_tick();
};

} //namespace Rinx

#endif // CLOCK_H
