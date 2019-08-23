#include "Reactor.h"
#include "../3rd/NanoLog/NanoLog.h"

RxReactor::RxReactor(uint8_t id):_event_handlers{{nullptr}},
    _id(id),_is_running(false)

{

}

uint8_t RxReactor::get_id() const noexcept
{
    return _id;
}

RxTimerHeap &RxReactor::get_timer_queue() noexcept
{
    return _timer_queue;
}

bool RxReactor::init()
{
    return _reactor_epoll.create(512);
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
    while(this->_is_running){
        int nfds=_reactor_epoll.wait(nullptr);
        if(nfds<0){
            LOG_WARN<<"_reactor_epoll wait failed, reactor_id="<<_id;
            return -1;
        }
        else if(nfds==0){
            if(this->_on_timeout){
                this->_on_timeout(this);
                continue;
            }
        }

        epoll_event *ep_events=_reactor_epoll.get_epoll_events();
        for(int i=0;i<nfds;i++){
            RxEvent rx_event;
            rx_event.Fd.raw_fd=static_cast<int>(ep_events[i].data.u64);
            rx_event.Fd.fd_type=static_cast<RxFDType>(ep_events[i].data.u64>>32);
            rx_event.reactor=this;

            std::vector<RxEventType> rx_event_types=RxReactorEpoll::get_rx_event_types(ep_events[i]);
            for(auto rx_ev_type:rx_event_types){
                EventHandler &ev_handler=_event_handlers[rx_ev_type][rx_event.Fd.fd_type];
                if(ev_handler&&ev_handler(rx_event)<0){
                     LOG_WARN<<"exec epoll handler failed. event_type="<<rx_ev_type<<
                               "fd="<<rx_event.Fd.raw_fd<<" fd_type="<<rx_event.Fd.fd_type;
                }
            }
        }
    }
    return 0;
}


