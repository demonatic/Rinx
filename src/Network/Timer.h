#ifndef TIMER_H
#define TIMER_H

#include <functional>
#include "../Util/Clock.h"

class RxReactor;

using TimerID=uint64_t;
using TimerCallback=std::function<void()>;

/// The caller need to hold and manage the RxTimer object
class RxTimer
{
public:
    RxTimer(RxReactor *reactor);

    TimerID start_timer(uint64_t milliseconds,TimerCallback expiry_action,bool repeat);
    void stop();

    bool is_active() const noexcept;
    bool is_repeat() const noexcept;

    TimerID get_id() const noexcept;
    uint64_t get_duration() const noexcept;

private:
    void expired();
    void set_active(bool active) noexcept;

    friend class RxTimerHeap;

    RxReactor *_reactor_belongs;

    TimerID _id;

    uint64_t _duration;
    std::size_t _heap_index;

    bool _is_active;
    bool _is_repeat;

    TimerCallback _cb;

};

#endif // TIMER_H
