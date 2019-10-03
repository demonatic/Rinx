#include <cstring>
#include <unistd.h>
#include "EventPoller.h"
#include "EventLoop.h"
#include "../3rd/NanoLog/NanoLog.h"

RxEventPoller::RxEventPoller():_epoll_fd(-1),_events(nullptr),_max_event_num(0)
{

}

RxEventPoller::~RxEventPoller()
{
    if(is_initialized()){
        destroy();
    }
}

bool RxEventPoller::create(int max_event_num)
{
    if(is_initialized())
        return true;

    _epoll_fd=::epoll_create(1);
    if(_epoll_fd<0){
        LOG_WARN<<"create epoll failed. Error"<<errno<<strerror(errno);
        return false;
    }

    _events=static_cast<epoll_event*>(malloc(sizeof(struct epoll_event)*max_event_num));
    if(_events==nullptr){
        LOG_WARN<<"malloc epoll events failed";
        return false;
    }
    _max_event_num=max_event_num;
    return true;
}

bool RxEventPoller::is_initialized() const noexcept
{
    return _epoll_fd!=-1&&_events!=nullptr;
}

void RxEventPoller::destroy() noexcept
{
    if(_epoll_fd!=-1){
        ::close(_epoll_fd);
         _epoll_fd=-1;
    }

    if(_events!=nullptr){
        free(_events);
        _events=nullptr;
    }
}

int RxEventPoller::wait(const int timeout_millsec)
{
    int nfds=::epoll_wait(_epoll_fd,_events,_max_event_num,timeout_millsec);
    if(nfds==0){
        nfds=Epoll_Timeout;
    }
    else if(nfds<0){
        nfds=errno==EINTR?Epoll_Interrupted:Epoll_Error;
    }
    return nfds;
}


epoll_event *RxEventPoller::get_epoll_events() const noexcept
{
    return _events;
}

bool RxEventPoller::add_fd_event(const RxFD Fd,const std::vector<RxEventType> &event_type)
{
    struct epoll_event epoll_event;
    bzero(&epoll_event,sizeof(struct epoll_event));
    set_ep_event(epoll_event,event_type);
    epoll_event.data.u64=(static_cast<uint64_t>(Fd.fd_type)<<32)|static_cast<uint64_t>(Fd.raw_fd);

    if(::epoll_ctl(_epoll_fd,EPOLL_CTL_ADD,Fd.raw_fd,&epoll_event)==-1){
        LOG_WARN<<"epoll_ctl add error: "<<errno<<" "<<strerror(errno);
        return false;
    }
    return true;
}

bool RxEventPoller::del_fd_event(const RxFD Fd)
{
    if(::epoll_ctl(_epoll_fd,EPOLL_CTL_DEL,Fd.raw_fd,nullptr)<0){
        LOG_WARN<<"epoll_ctl del error"<<errno<<" "<<strerror(errno);
        return false;
    }
    return true;
}

std::vector<RxEventType> RxEventPoller::get_rx_event_types(const epoll_event &ep_event) noexcept
{
    std::vector<RxEventType> rx_event_types;
    uint32_t evt=ep_event.events;

    /// 1. if remote callshutdown(SHUT_RD), epoll will return EPOLLIN | EPOLLRDHUP
    /// 2. if remote call shutdown(SHUT_RDWR), epoll will return EPOLLIN | EPOLLRDHUP | EPOLLHUP
    /// 3. if remote send RST, epoll will return EPOLLIN | EPOLLERR | EPOLLHUP
    /// 4. if remote only shutdown(SHUT_WR), epoll will return nothing

    if(evt&(EPOLLHUP|EPOLLERR|EPOLLRDHUP)){
       rx_event_types.push_back(Rx_EVENT_ERROR);
    }
    if(evt&EPOLLIN){
        rx_event_types.push_back(Rx_EVENT_READ);
    }
    if(evt&EPOLLOUT){
        rx_event_types.push_back(Rx_EVENT_WRITE);
    }

    return rx_event_types;
}

void RxEventPoller::set_ep_event(epoll_event &ep_event,const std::vector<RxEventType> &rx_events) noexcept
{
    ep_event.events|=EPOLLET;
    for(RxEventType rx_event:rx_events){
        if(rx_event==Rx_EVENT_READ){
            ep_event.events|=EPOLLIN;
        }
        else if(rx_event==Rx_EVENT_WRITE){
            ep_event.events|=EPOLLOUT;
        }
    }

}