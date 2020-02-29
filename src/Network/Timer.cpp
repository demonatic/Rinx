#include "Rinx/Network/Timer.h"
#include "Rinx/Network/EventLoop.h"
#include "Rinx/Network/TimerHeap.h"

namespace Rinx {

RxTimer::RxTimer(RxEventLoop *eventloop):_id(std::numeric_limits<uint64_t>::max()),
    _heap_index(std::numeric_limits<size_t>::max()),_duration(0),
    _is_active(false),_is_repeat(false),_eventloop_belongs(eventloop)
{

}

RxTimer::~RxTimer()
{
    if(this->is_active()){
        stop();
    }
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
    _timeout_cb=expiry_action;
    timer_heap.add_timer(expire_time,this);
    this->set_active(true);
    return _id;
}

void RxTimer::stop()
{
    RxTimerHeap &timer_heap=_eventloop_belongs->get_timer_heap();
    timer_heap.remove_timer(this->_id);

    this->set_active(false);
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
    return _duration;
}

void RxTimer::expired()
{
    this->_timeout_cb();
}

void RxTimer::set_active(bool active) noexcept
{
    this->_is_active=active;
}

} //namespace Rinx
