#include "Util/Clock.h"
#include "Util/Util.h"
#include "3rd/NanoLog/NanoLog.h"

namespace Rinx {

uint64_t Clock::get_now_tick()
{
    timespec time_spec;
    if(unlikely(clock_gettime(CLOCK_MONOTONIC,&time_spec))<0){
        LOG_WARN<<"get clock time err: "<<errno;
    }
    return time_spec.tv_sec*1000u+time_spec.tv_nsec/1000000u;
}

} //namespace Rinx
