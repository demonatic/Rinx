#include "Reactor.h"
#include "Socket.h"
#include "../Util/Util.h"
#include "../3rd/NanoLog/NanoLog.h"

RxReactor::RxReactor(uint8_t id):_id(id),_is_running(false),
    _event_handlers{{nullptr}}

{

}

bool RxReactor::init()
{
    if(!_reactor_epoll.create(512)){
        return false;
    }

    _event_fd={Rx_FD_EVENT,RxSock::create_event_fd()};
    if(_event_fd.raw_fd==-1){
        LOG_WARN<<"create event fd failed";
        return false;
    }

    //register event fd read handler
    set_event_handler(_event_fd.fd_type,Rx_EVENT_READ,[this](const RxEvent &event_data)->RxHandlerRes{
        if(!RxSock::read_event_fd(event_data.Fd.raw_fd)){
            LOG_WARN<<"read event fd failed, reactor_id="<<_id;
            return RX_HANDLER_ERR;
        }
        return Rx_HANDLER_OK;
    });
    monitor_fd_event(_event_fd,{Rx_EVENT_READ});

    return true;
}

bool RxReactor::monitor_fd_event(const RxFD Fd, const std::vector<RxEventType> &rx_events)
{
    return this->_reactor_epoll.add_fd_event(Fd,rx_events);
}

bool RxReactor::unmonitor_fd_event(const RxFD Fd)
{
    return this->_reactor_epoll.del_fd_event(Fd);
}

bool RxReactor::set_event_handler(RxFDType fd_type, RxEventType event_type, EventHandler handler) noexcept
{
    if(event_type>RxEventType::Rx_EVENT_TYPE_MAX||fd_type>RxFDType::Rx_FD_TYPE_MAX){
        LOG_WARN<<"invalid event_type or fd_type";
        return false;
    }
    _event_handlers[event_type][fd_type]=handler;
    return true;
}

bool RxReactor::remove_event_handler(RxFDType fd_type, RxEventType event_type) noexcept
{
    if(event_type>RxEventType::Rx_EVENT_TYPE_MAX||fd_type>RxFDType::Rx_FD_TYPE_MAX){
        LOG_WARN<<"invalid event_type or fd_type";
        return false;
    }
    _event_handlers[event_type][fd_type]=nullptr;
    return true;
}

int RxReactor::start_event_loop()
{
    _is_running=true;
    LOG_INFO<<"start event loop";

    while(_is_running){
        loop_each_begin();
        check_timers();
        poll_and_dispatch_io();
        run_defers();
        loop_each_end();
    }
    return 0;
}


void RxReactor::queue_work(RxReactor::DeferCallback cb)
{
    {
        RxMutexLockGuard lock(_defer_mutex);
        _defer_functors.push_back(std::move(cb));
    }
    wake_up_loop();
}

void RxReactor::set_loop_each_begin(ReactorCallback loop_each_begin)
{
    this->_on_loop_each_begin=loop_each_begin;
}

void RxReactor::set_loop_each_end(ReactorCallback loop_each_end)
{
    this->_on_loop_each_end=loop_each_end;
}

void RxReactor::wake_up_loop()
{
    if(!RxSock::write_event_fd(_event_fd.raw_fd)){
        LOG_WARN<<"reactor write event fd failed, reactor_id="<<_id;
    }
}

void RxReactor::loop_each_begin()
{
    if(this->_on_loop_each_begin){
        _on_loop_each_begin(this);
    }
}

void RxReactor::loop_each_end()
{
    if(_on_loop_each_end){
        _on_loop_each_end(this);
    }
}

int RxReactor::poll_and_dispatch_io()
{
    int nfds=_reactor_epoll.wait();

    switch(nfds){
        case Epoll_Error:
            LOG_WARN<<"reactor_epoll wait failed, reactor_id="<<_id;
            return -1;
        case Epoll_Timeout:
            if(_on_timeout)
                _on_timeout(this);
            return 0;
        case Epoll_Interrupted:
            return 0;
    }

    int dispatched=0;
    epoll_event *ep_events=_reactor_epoll.get_epoll_events();

    for(int i=0;i<nfds;i++){
        RxEvent rx_event;
        rx_event.Fd.raw_fd=static_cast<int>(ep_events[i].data.u64);
        rx_event.Fd.fd_type=static_cast<RxFDType>(ep_events[i].data.u64>>32);
        rx_event.reactor=this;

        std::vector<RxEventType> rx_event_types=RxReactorEpoll::get_rx_event_types(ep_events[i]);
        for(auto rx_ev_type:rx_event_types){
            EventHandler &ev_handler=_event_handlers[rx_ev_type][rx_event.Fd.fd_type];

            if(ev_handler&&ev_handler(rx_event)==Rx_HANDLER_OK){
                 ++dispatched;
            }
            else{
                LOG_WARN<<"io_poll handler doesn't exist or exec failed. "
                "event_type="<<rx_ev_type<<" fd_type="<<rx_event.Fd.fd_type<<" fd="<<rx_event.Fd.raw_fd;
            }
        }
    }
    return dispatched;
}

int RxReactor::check_timers()
{
    return _timer_heap.check_timer_expiry();
}

void RxReactor::run_defers()
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

uint8_t RxReactor::get_id() const noexcept
{
    return _id;
}

RxTimerHeap &RxReactor::get_timer_heap() noexcept
{
    return _timer_heap;
}
