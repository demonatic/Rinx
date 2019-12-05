#ifndef EVENTLOOP_H
#define EVENTLOOP_H

#include <functional>
#include <memory>
#include <vector>
#include <list>
#include <any>
#include <atomic>
#include "EventPoller.h"
#include "TimerHeap.h"
#include "../RinxDefines.h"
#include "../Util/Mutex.h"
#include "../Util/ThreadPool.h"
#include "../Util/Singeleton.h"
#include "../Server/Signal.h"

using g_threadpool=RxSingeleton<RxThreadPool>;
///
/// \brief The RxEventLoop class is an Reactor that manage handles,it includes:
///     1.handles: to represents sockets or fds
///     2.Synchronous Event Demultiplexer: Epoll Object
///     3.Event handler: interface to handle events
///     4.Timers management
///     5.async task post
class RxEventLoop
{
public:
    using LoopCallback=std::function<void(RxEventLoop*)>;
    using DeferCallback=std::function<void()>;

public:
    RxEventLoop(uint8_t id);

    uint8_t get_id() const noexcept;
    RxTimerHeap& get_timer_heap() noexcept;

    bool init();

    bool register_fd(const RxFD Fd,const std::vector<RxEventType> &rx_events);
    bool unregister_fd(const RxFD Fd);

    void set_event_handler(RxFDType fd_type,RxEventType event_type,EventHandler handler) noexcept;
    void remove_event_handler(RxFDType fd_type,RxEventType event_type) noexcept;

    int start_event_loop();
    /// @brief stop eventloop asynchronously
    void stop_event_loop();

    void queue_work(DeferCallback cb);

    void set_loop_prepare(LoopCallback loop_prepare_cb) noexcept;

    template<typename F1,typename F2,typename ...Args>
    std::enable_if_t<false==std::is_same_v<void,std::invoke_result_t<F1,Args...>>>
    async(F1 &&task_func,F2 &&finish_cb,Args&& ...task_args){
        g_threadpool::get_instance()->post([this,task_func,&task_args...,finish_cb](){
            using task_ret_t=std::invoke_result_t<F1,Args...>;
            //TODO is it safe to reference as they are in a different call stack?
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
    int poll_and_dispatch_io(int timeout_millsec);
    int check_timers();
    void run_defers();
    void do_prepare();
    int get_poll_timeout();

    void quit();

private:
    uint8_t _id;
    std::atomic_bool _is_running;

    LoopCallback _on_loop_prepare;

    RxEventPoller _event_poller;
    LoopCallback _on_timeout;
    EventHandler _event_handlers[RxEventType::__Rx_EVENT_TYPE_MAX][RxFDType::__RxFD_TYPE_COUNT];

    RxFD _event_fd;

    RxTimerHeap _timer_heap;

    RxMutex _defer_mutex;
    std::vector<DeferCallback> _defer_functors;

};


#endif // EVENTLOOP_H
