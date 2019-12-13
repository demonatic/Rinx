#ifndef REACTOREPOLL_H
#define REACTOREPOLL_H

#include <sys/epoll.h>
#include <sys/time.h>
#include <vector>
#include <functional>
#include "../Util/Noncopyable.h"
#include "FD.h"

enum RxEventType:uint8_t{
    Rx_EVENT_READ=0,
    Rx_EVENT_WRITE,
    Rx_EVENT_ERROR,

    __Rx_EVENT_TYPE_MAX
};

class RxEventLoop;
struct RxEvent{
    RxFD Fd;
    RxEventType event_type;
    RxEventLoop *eventloop;
};

using EventHandler=std::function<bool(const RxEvent &event_data)>;

class RxEventPoller:public RxNoncopyable
{
public:
    static constexpr int Epoll_Timeout=0,Epoll_Interrupted=-1,Epoll_Error=-2;

public:
    RxEventPoller();
    ~RxEventPoller();

    bool create(int max_event_num);
    bool is_initialized() const noexcept;
    void destroy() noexcept;

    int wait(int timeout_millsec=-1);

    epoll_event *get_epoll_events() const noexcept;

    /// @brief add fd to monitor the specific event(s) on it
    bool add_fd_event(const RxFD Fd,const std::vector<RxEventType> &event_type);
    bool del_fd_event(const RxFD Fd);

    std::vector<RxEventType> get_rx_event_types(const epoll_event &ep_event) const noexcept;
    void set_ep_event(epoll_event &ep_event,const std::vector<RxEventType> &rx_events) const noexcept;

private:
    RxFD _epoll_fd;
    struct epoll_event *_ep_events;

    int _max_event_num;
};


#endif // REACTOREPOLL_H
