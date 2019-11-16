#include "EventLoop.h"
#include "FD.h"
#include "../Util/Util.h"
#include "../3rd/NanoLog/NanoLog.h"
#include <string.h>

RxEventLoop::RxEventLoop(uint8_t id):_id(id),_is_running(false),
    _event_handlers{{nullptr}} //FIX THIS

{

}

bool RxEventLoop::init()
{
    if(!_eventloop_epoll.create(512)){
        return false;
    }

    _event_fd=RxFDHelper::Event::create_event_fd();
    if(_event_fd.raw==-1){
        LOG_WARN<<"create event fd failed";
        return false;
    }

    //register event fd read handler
    set_event_handler(_event_fd.type,Rx_EVENT_READ,[this](const RxEvent &event_data)->RxHandlerRc{
        if(!RxFDHelper::Event::read_event_fd(event_data.Fd)){
            LOG_WARN<<"read event fd failed, eventloop_id="<<_id;
            return RX_HANDLER_ERR;
        }
        return Rx_HANDLER_OK;
    });
    monitor_fd_event(_event_fd,{Rx_EVENT_READ});

    return true;
}

bool RxEventLoop::monitor_fd_event(const RxFD Fd, const std::vector<RxEventType> &rx_events)
{
    return this->_eventloop_epoll.add_fd_event(Fd,rx_events);
}

bool RxEventLoop::unmonitor_fd_event(const RxFD Fd)
{
    return this->_eventloop_epoll.del_fd_event(Fd);
}

bool RxEventLoop::set_event_handler(RxFDType fd_type, RxEventType event_type, EventHandler handler) noexcept
{
    if(event_type>RxEventType::Rx_EVENT_TYPE_MAX||fd_type>RxFDType::__RxFD_TYPE_COUNT){
        LOG_WARN<<"invalid event_type or fd_type";
        return false;
    }
    _event_handlers[event_type][fd_type]=handler;
    return true;
}

bool RxEventLoop::remove_event_handler(RxFDType fd_type, RxEventType event_type) noexcept
{
    if(event_type>RxEventType::Rx_EVENT_TYPE_MAX||fd_type>RxFDType::__RxFD_TYPE_COUNT){
        LOG_WARN<<"invalid event_type or fd_type";
        return false;
    }
    _event_handlers[event_type][fd_type]=nullptr;
    return true;
}

int RxEventLoop::start_event_loop()
{
    _is_running=true;
    LOG_INFO<<"START EventLoop "<<get_id();

    while(_is_running){
        do_prepare();
        check_timers();

        int timeout=get_poll_timeout();
        poll_and_dispatch_io(timeout);
        run_defers();
    }
    quit();
    return 0;
}

void RxEventLoop::stop_event_loop()
{
    _is_running=false;
    wake_up_loop();
}


void RxEventLoop::queue_work(RxEventLoop::DeferCallback cb)
{
    {
        RxMutexLockGuard lock(_defer_mutex);
        _defer_functors.push_back(std::move(cb));
    }
    wake_up_loop();
}

void RxEventLoop::set_loop_prepare(RxEventLoop::ReactorCallback loop_prepare_cb) noexcept
{
    _on_loop_prepare=loop_prepare_cb;
}


void RxEventLoop::wake_up_loop()
{
    if(!RxFDHelper::Event::write_event_fd(_event_fd)){
        LOG_WARN<<"eventloop write event fd failed, eventloop_id="<<_id<<" Reason:"<<errno<<' '<<strerror(errno);
    }
}

void RxEventLoop::do_prepare()
{
    if(this->_on_loop_prepare){
        _on_loop_prepare(this);
    }
}

int RxEventLoop::get_poll_timeout()
{
    uint64_t timeout=_timer_heap.get_timeout_interval();
    return timeout!=0?static_cast<int>(timeout):-1;
}

void RxEventLoop::quit()
{
    RxFDHelper::close(_event_fd);
    _eventloop_epoll.destroy();
    LOG_INFO<<"QUIT EventLoop "<<get_id();
}

int RxEventLoop::poll_and_dispatch_io(int timeout_millsec)
{
    int nfds=_eventloop_epoll.wait(timeout_millsec);

    switch(nfds){
        case Epoll_Error:
            LOG_WARN<<"eventloop_epoll wait failed, eventloop_id="<<_id;
            return -1;
        case Epoll_Timeout:
            if(_on_timeout)
                _on_timeout(this);
            return 0;
        case Epoll_Interrupted:
            return 0;
    }

    int dispatched=0;
    epoll_event *ep_events=_eventloop_epoll.get_epoll_events();

    for(int i=0;i<nfds;i++){
        RxEvent rx_event;
        rx_event.Fd.raw=static_cast<int>(ep_events[i].data.u64);
        rx_event.Fd.type=static_cast<RxFDType>(ep_events[i].data.u64>>32);
        rx_event.eventloop=this;

        const std::vector<RxEventType> rx_event_types=RxEventPoller::get_rx_event_types(ep_events[i]);
        for(auto rx_ev_type:rx_event_types){
            EventHandler &ev_handler=_event_handlers[rx_ev_type][rx_event.Fd.type];
            if(!ev_handler)
                continue;

            RxHandlerRc rc=ev_handler(rx_event);
            if(rc==Rx_HANDLER_OK){
                 ++dispatched;
            }
            else if(rc==RX_HANDLER_ERR){
                LOG_WARN<<"io_poll handler doesn't exist or exec failed. "
                "event_type="<<rx_ev_type<<" fd_type="<<rx_event.Fd.type<<" fd="<<rx_event.Fd.raw;
            }
            else break;
        }
    }
    return dispatched;
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

uint8_t RxEventLoop::get_id() const noexcept
{
    return _id;
}

RxTimerHeap &RxEventLoop::get_timer_heap() noexcept
{
    return _timer_heap;
}
