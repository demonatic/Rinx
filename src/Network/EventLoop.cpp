#include "Network/EventLoop.h"
#include "Network/FD.h"
#include "Util/Util.h"
#include "3rd/NanoLog/NanoLog.hpp"
#include <string.h>

namespace Rinx {

RxEventLoop::RxEventLoop(uint8_t id):_id(id),_is_running(false),_handler_table{}
{

}

bool RxEventLoop::init()
{
    if(!_event_poller.create(512)){
        return false;
    }

    _event_fd=FDHelper::Event::create_event_fd();
    if(_event_fd.raw==-1){
        LOG_WARN<<"create event fd failed";
        return false;
    }

    //register event fd read handler
    set_event_handler(_event_fd.type,Rx_EVENT_READ,[this](const RxEvent &event_data){
        if(!FDHelper::Event::read_event_fd(event_data.Fd)){
            LOG_WARN<<"read event fd failed, eventloop_id="<<_id;
            return false;
        }
        return true;
    });
    monitor_fd(_event_fd,{Rx_EVENT_READ});

    return true;
}

int RxEventLoop::start_event_loop()
{
    _is_running=true;
    LOG_INFO<<"START EventLoop "<<get_id();

    while(_is_running){
        check_timers();
        auto timeout=get_poll_timeout();

        poll_and_dispatch_io(timeout);
        run_defers();
        on_finish();
    }
    quit();
    return 0;
}

void RxEventLoop::stop_event_loop()
{
    _is_running=false;
    usleep(100); //to see if loop has quited
    if(_event_fd!=RxInvalidFD){
        wake_up_loop();
    }
}


void RxEventLoop::queue_work(RxEventLoop::DeferCallback cb)
{
    {
        RxMutexLockGuard lock(_defer_mutex);
        _defer_functors.push_back(std::move(cb));
    }
    wake_up_loop();
}

void RxEventLoop::wake_up_loop()
{
    if(!FDHelper::Event::write_event_fd(_event_fd)){
        LOG_WARN<<"eventloop write event fd failed, eventloop_id="<<_id<<" Reason:"<<errno<<' '<<strerror(errno);
    }
}

int RxEventLoop::get_poll_timeout()
{
    uint64_t timeout=_timer_heap.get_timeout_interval();
    return timeout!=0?static_cast<int>(timeout):-1;
}

void RxEventLoop::quit()
{
    FDHelper::close(_event_fd);
    _event_poller.destroy();
    LOG_INFO<<"QUIT EventLoop "<<get_id();
}

int RxEventLoop::poll_and_dispatch_io(int timeout_millsec)
{
    int nfds=_event_poller.wait(timeout_millsec);

    if(nfds>0){
        int dispatched=0;
        epoll_event *ep_events=_event_poller.get_epoll_events();

        for(int i=0;i<nfds;i++){
            RxEvent rx_event;
            rx_event.Fd.raw=static_cast<int>(ep_events[i].data.u64);
            rx_event.Fd.type=static_cast<RxFDType>(ep_events[i].data.u64>>32);
            rx_event.eventloop=this;

            const std::vector<RxEventType> rx_event_types=_event_poller.get_rx_event_types(ep_events[i]);
            for(RxEventType rx_event_type:rx_event_types){
                rx_event.event_type=rx_event_type;
                EventHandler event_handler=_handler_table[rx_event.event_type][rx_event.Fd.type];
                if(!event_handler)
                    continue;

                if(!event_handler(rx_event))
                    break;

                ++dispatched;
            }
        }
        return dispatched;
    }

    switch(nfds){
        case RxEventPoller::Epoll_Error:
            LOG_WARN<<"eventloop_epoll wait failed, eventloop_id="<<_id;
            return -1;
        case RxEventPoller::Epoll_Timeout:
            if(_on_timeout){
                _on_timeout(this);
            }
            return 0;
        case RxEventPoller::Epoll_Interrupted:
            return 0;
        default:
            return -1;
    }

}

int RxEventLoop::check_timers()
{
    return _timer_heap.check_timer_expiry();
}

void RxEventLoop::run_defers()
{
    std::vector<DeferCallback> defers;
    {
        RxMutexLockGuard lock(_defer_mutex);
        defers.swap(_defer_functors);
    }

    for(const DeferCallback &defer_cb:defers){
        defer_cb();
    }
}

bool RxEventLoop::monitor_fd(const RxFD fd, const std::vector<RxEventType> &events)
{
    return this->_event_poller.add_fd_event(fd,events);
}

bool RxEventLoop::unmonitor_fd(const RxFD fd)
{
    return this->_event_poller.del_fd_event(fd);
}

void RxEventLoop::set_event_handler(RxFDType fd_type, RxEventType event_type, EventHandler handler) noexcept
{
    _handler_table[event_type][fd_type]=handler;
}

void RxEventLoop::remove_event_handler(RxFDType fd_type, RxEventType event_type) noexcept
{
    _handler_table[event_type][fd_type]=nullptr;
}

void RxEventLoop::set_loop_finish(RxEventLoop::LoopCallback loop_finish_cb) noexcept
{
    _on_loop_finish=loop_finish_cb;
}

void RxEventLoop::on_finish()
{
    if(_on_loop_finish){
        _on_loop_finish(this);
    }
}

uint8_t RxEventLoop::get_id() const noexcept
{
    return _id;
}

RxTimerHeap &RxEventLoop::get_timer_heap() noexcept
{
    return _timer_heap;
}

} //namespace Rinx
