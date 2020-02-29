#ifndef TIMER_H
#define TIMER_H

#include <functional>
#include "Rinx/Util/Clock.h"
#include "Rinx/Util/Noncopyable.h"

namespace Rinx {

class RxEventLoop;

using TimerID=uint64_t;
using TimerCallback=std::function<void()>;

/// A RAII oneshot/repeated Timer class
/// The caller need to hold the RxTimer object
class RxTimer
{
    friend class RxTimerHeap;
public:
    RxTimer(RxEventLoop *eventloop);
    ~RxTimer();

    TimerID start_timer(uint64_t milliseconds,TimerCallback expiry_action,bool repeat=false);
    void stop();

    bool is_active() const noexcept;
    bool is_repeat() const noexcept;

    TimerID get_id() const noexcept;
    uint64_t get_duration() const noexcept;

private:    
    void expired();
    void set_active(bool active) noexcept;

private:
    TimerID _id;
    size_t _heap_index;

    uint64_t _duration;

    bool _is_active;
    bool _is_repeat;

    TimerCallback _timeout_cb;

    RxEventLoop *_eventloop_belongs;
};

} //namespace Rinx
#endif // TIMER_H
