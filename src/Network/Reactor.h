#ifndef REACTOR_H
#define REACTOR_H

#include <memory>
#include <functional>
#include <vector>
#include <list>
#include "ReactorEpoll.h"

///
/// \brief The RxReactor class is used to manage handles,it includes:
///     1.handles: to represents sockets or fds
///     2.Synchronous Event Demultiplexer: Epoll Object
///     3.Event handler: interface to handle events
///     4.Concrete Event handler: implement some logic of an application to handle specific event
///
class RxReactor
{
public:
    RxReactor(uint8_t id);
    uint8_t get_id() const noexcept;

    bool init();

    bool monitor_fd_event(const RxFD Fd,const std::list<RxEventType> &event_types);
    bool unmonitor_fd_event(const RxFD Fd);

    bool set_event_handler(RxFDType fd_type,RxEventType event_type,EventHandler handler) noexcept;
    bool remove_event_handler(RxFDType fd_type,RxEventType event_type) noexcept;

    int start_event_loop();


private:
    int wait();

private:
    RxReactorEpoll _reactor_epoll;

    EventHandler _event_handlers[RxEventType::Rx_MAX_EVENT_TYPE][RxFDType::Rx_MAX_FD_TYPE];

    std::function<void(RxReactor*)> _on_timeout;

    uint8_t _id;
    bool _is_running;
};


#endif // REACTOR_H
