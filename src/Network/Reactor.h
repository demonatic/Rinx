#ifndef REACTOR_H
#define REACTOR_H

#include <memory>
#include <functional>
#include <vector>
#include <list>
#include <any>
#include "ReactorEpoll.h"
#include "TimerHeap.h"
#include "../RinxDefines.h"
#include "../Util/Mutex.h"

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
    using PollTimeOutCallback=std::function<void(RxReactor *)>;
    using DeferCallback=std::function<void()>;

public:
    RxReactor(uint8_t id);

    uint8_t get_id() const noexcept;
    RxTimerHeap& get_timer_heap() noexcept;

    bool init();

    bool monitor_fd_event(const RxFD Fd,const std::vector<RxEventType> &rx_events);
    bool unmonitor_fd_event(const RxFD Fd);

    bool set_event_handler(RxFDType fd_type,RxEventType event_type,EventHandler handler) noexcept;
    bool remove_event_handler(RxFDType fd_type,RxEventType event_type) noexcept;

    int start_event_loop();

    void queue_work(DeferCallback cb);


    template<typename F1,typename F2,typename ...Args>
    std::enable_if_t<false==std::is_same_v<void,std::invoke_result_t<F1,Args...>>>
    async(F1 &&task_func,F2 &&finish_cb,Args&& ...task_args){
        g_threadpool::get_instance()->post([this,task_func,&task_args...,finish_cb](){
            using task_ret_t=std::invoke_result_t<F1,Args...>;
            //TODO it is safe to reference as they are in a different call stack?
            task_ret_t res=task_func(std::forward<Args...>(task_args)...);
            this->queue_work([finish_cb,&res](){
                finish_cb(res);
            });
        });
    }

    template<typename F1,typename F2,typename ...Args>
    std::enable_if_t<true==std::is_same_v<void,std::invoke_result_t<F1,Args...>>>
    async(F1 &&task_func,F2 &&finish_cb,Args&& ...task_args){
        g_threadpool::get_instance()->post([this,task_func,&task_args...,finish_cb](){
            task_func(std::forward<Args...>(task_args)...);
            this->queue_work([finish_cb](){
                finish_cb();
            });
        });
    }

    void wake_up_loop();

private:
    int poll_and_dispatch_io();
    int check_timers();
    void run_defers();

private:

    uint8_t _id;
    bool _is_running;

    RxReactorEpoll _reactor_epoll;
    PollTimeOutCallback _on_timeout;
    EventHandler _event_handlers[RxEventType::Rx_EVENT_TYPE_MAX][RxFDType::Rx_FD_TYPE_MAX];

    RxFD _event_fd;

    RxTimerHeap _timer_heap;

    RxMutex _defer_mutex;

    std::vector<DeferCallback> _defer_functors;
};


#endif // REACTOR_H
