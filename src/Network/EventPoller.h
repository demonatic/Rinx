#ifndef REACTOREPOLL_H
#define REACTOREPOLL_H

#include <sys/epoll.h>
#include <sys/time.h>
#include <vector>
#include <functional>
#include "FD.h"

enum RxEventType:uint32_t{
    Rx_EVENT_READ=0,
    Rx_EVENT_WRITE,
    Rx_EVENT_ERROR,

    Rx_EVENT_TYPE_MAX
};

class RxEventLoop;
struct RxEvent{
    RxFD Fd;
    RxEventLoop *eventloop;
};

enum RxHandlerRc{
    Rx_HANDLER_OK=0,
    Rx_HANDLER_EXIT_ALL,
    RX_HANDLER_ERR
};
using EventHandler=std::function<RxHandlerRc(const RxEvent &event_data)>;

#define Epoll_Timeout 0
#define Epoll_Interrupted -1
#define Epoll_Error -2

class RxEventLoop;
class RxEventPoller
{
public:
    RxEventPoller();
    ~RxEventPoller();
    RxEventPoller(const RxEventPoller&)=delete;
    RxEventPoller& operator=(const RxEventPoller&)=delete;

    bool create(int max_event_num);
    bool is_initialized() const noexcept;
    void destroy() noexcept;

    int wait(const int timeout_millsec=-1);

    epoll_event *get_epoll_events() const noexcept;

    /// @brief add fd to monitor the specific event(s) on it
    bool add_fd_event(const RxFD Fd,const std::vector<RxEventType> &event_type);
    bool del_fd_event(const RxFD Fd);

    static std::vector<RxEventType> get_rx_event_types(const epoll_event &ep_event) noexcept;
    static void set_ep_event(epoll_event &ep_event,const std::vector<RxEventType> &rx_events) noexcept;

private:
    int _epoll_fd;
    struct epoll_event *_events;

    int _max_event_num;
};


#endif // REACTOREPOLL_H
