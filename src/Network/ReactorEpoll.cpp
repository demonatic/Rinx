#include <string.h>
#include <unistd.h>
#include "ReactorEpoll.h"
#include "Reactor.h"
#include "../3rd/NanoLog/NanoLog.h"

RxReactorEpoll::RxReactorEpoll():_epoll_fd(-1),_events(nullptr)
{

}

RxReactorEpoll::~RxReactorEpoll()
{
    if(is_initialized()){
        destroy();
    }
}

bool RxReactorEpoll::create(int max_event_num)
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

bool RxReactorEpoll::is_initialized() const noexcept
{
    return _epoll_fd!=-1&&_events!=nullptr;
}

void RxReactorEpoll::destroy() noexcept
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

int RxReactorEpoll::wait(const struct timeval *timeout)
{
    int timeo_millsec=timeout==nullptr?0:timeout->tv_sec*1000+timeout->tv_usec/1000;
    int nfds;
    do{
        nfds=::epoll_wait(_epoll_fd,_events,_max_event_num,timeo_millsec);
    }while(nfds<0&&errno==EINTR);

    return nfds;
}

epoll_event *RxReactorEpoll::get_epoll_events() const noexcept
{
    return _events;
}

bool RxReactorEpoll::add_fd_event(const RxFD Fd,const std::vector<RxEventType> &event_type)
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

bool RxReactorEpoll::del_fd_event(const RxFD Fd)
{
    if(::epoll_ctl(_epoll_fd,EPOLL_CTL_DEL,Fd.raw_fd,nullptr)<0){
        LOG_WARN<<"epoll_ctl del error"<<errno<<" "<<strerror(errno);
        return false;
    }
    return true;
}

std::vector<RxEventType> RxReactorEpoll::get_rx_event_types(const epoll_event &ep_event) noexcept
{
    std::vector<RxEventType> rx_event_types;
    uint32_t evt=ep_event.events;

    if(evt&EPOLLIN){
        rx_event_types.push_back(Rx_EVENT_READ);
    }
    if(evt&EPOLLOUT){
        rx_event_types.push_back(Rx_EVENT_WRITE);
    }
    if(evt&(EPOLLHUP|EPOLLERR|EPOLLRDHUP)){
        //ignore ERR and HUP, because event is already processed at IN and OUT handler.
        if(!((evt&EPOLLIN)||(evt&EPOLLOUT))){
            rx_event_types.push_back(Rx_EVENT_ERROR);
        }
    }
    return rx_event_types;
}

void RxReactorEpoll::set_ep_event(epoll_event &ep_event,const std::vector<RxEventType> &rx_events) noexcept
{
    for(RxEventType rx_event:rx_events){
        if(rx_event==Rx_EVENT_READ){
            ep_event.events|=EPOLLIN|EPOLLET;
        }
        else if(rx_event==Rx_EVENT_WRITE){
            ep_event.events|=EPOLLOUT|EPOLLET;
        }
    }

}
