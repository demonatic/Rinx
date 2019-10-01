#include "Timer.h"
#include "EventLoop.h"
#include "TimerHeap.h"


RxTimer::RxTimer(RxEventLoop *eventloop):_eventloop_belongs(eventloop),
    _id(std::numeric_limits<uint64_t>::max()),_duration(0),
    _heap_index(std::numeric_limits<size_t>::max()),
    _is_active(false),_is_repeat(false)
{

}

TimerID RxTimer::start_timer(uint64_t milliseconds, TimerCallback expiry_action,bool repeat)
{
    uint64_t expire_time=Clock::get_now_tick()+milliseconds;
    RxTimerHeap &timer_heap=_eventloop_belongs->get_timer_heap();
    if(_is_active){
        this->stop();
    }

    _is_repeat=repeat;
    _duration=milliseconds;
    _cb=expiry_action;
    _id=timer_heap.add_timer(expire_time,this);

    return _id;
}

void RxTimer::stop()
{
    RxTimerHeap &timer_heap=_eventloop_belongs->get_timer_heap();
    timer_heap.remove_timer(this->_id);
}

bool RxTimer::is_active() const noexcept
{
    return _is_active;
}

bool RxTimer::is_repeat() const noexcept
{
    return _is_repeat;
}

TimerID RxTimer::get_id() const noexcept
{
    return _id;
}

uint64_t RxTimer::get_duration() const noexcept
{
    return  _duration;
}

void RxTimer::expired()
{
    this->_cb();
}

void RxTimer::set_active(bool active) noexcept
{
    this->_is_active=active;
}
