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
#include "Rinx/RinxDefines.h"
#include "Rinx/Util/Mutex.h"
#include "Rinx/Util/ThreadPool.h"
#include "Rinx/Util/Singeleton.h"
#include "Rinx/Server/Signal.h"

namespace Rinx {

using g_threadpool=RxSingeleton<RxThreadPool>;
///
/// \brief The RxEventLoop class is an Reactor that manage handles,it includes:
///     1.handles: to represents sockets or fds
///     2.Synchronous Event Demultiplexer: Epoll Object
///     3.Event handler: interface to handle events
///     4.Timers management
///     5.async task post
class RxEventLoop:RxNoncopyable
{
public:
    using LoopCallback=std::function<void(RxEventLoop*)>;
    using DeferCallback=std::function<void()>;

public:
    RxEventLoop(uint8_t id);
    ~RxEventLoop();

    uint8_t get_id() const noexcept;
    RxTimerHeap& get_timer_heap() noexcept;

    bool init();

    /// @brief watch any event within param events on this fd
    bool monitor_fd(const RxFD fd,const std::vector<RxEventType> &events);
    /// @brief unwatch any event on this fd
    bool unmonitor_fd(const RxFD fd);

    /// @brief set callback when fd of type fd_type has event of event_type occurs
    void set_event_handler(RxFDType fd_type,RxEventType event_type,EventHandler handler) noexcept;
    void remove_event_handler(RxFDType fd_type,RxEventType event_type) noexcept;

    int start_event_loop();

    /// @brief stop eventloop asynchronously
    void stop_event_loop();

    /// @brief thread safe function to queue a work into the eventloop,
    /// which will be executed after io handlers being dispatched
    void queue_work(DeferCallback cb);

    /// @brief set the callback each time a loop round finished
    void set_loop_finish(LoopCallback loop_finish_cb) noexcept;

    /// @brief post the task_func into a thread pool with task paramater ...task_args
    /// upon thread pool finish execution, queue the finish_cb which accept the execution result of task as a parameter
    template<typename F1,typename F2,typename ...Args>
    std::enable_if_t<false==std::is_same_v<void,std::invoke_result_t<F1,Args...>>>
    async(F1 &&task,F2 &&finish_callback,Args&& ...task_args){
        g_threadpool::get_instance()->post([this,task,&task_args...,finish_callback]() mutable{
            using task_ret_t=std::invoke_result_t<F1,Args...>;
            task_ret_t res=task(std::forward<Args>(task_args)...);
            this->queue_work([=]() mutable{
                finish_callback(res);
            });
        });
    }

    /// @brief same as above with task of none return value
    template<typename F1,typename F2,typename ...Args>
    std::enable_if_t<true==std::is_same_v<void,std::invoke_result_t<F1,Args...>>>
    async(F1 &&task,F2 &&finish_callback,Args&& ...task_args){
        g_threadpool::get_instance()->post([this,task,&task_args...,finish_callback]() mutable{
            task(std::forward<Args>(task_args)...);
            this->queue_work([=]() mutable{
                finish_callback();
            });
        });
    }

    ///@brief wake up the loop which possibily is being blocked at poll_and_dispatch_io
    void wake_up_loop();

private:
    int poll_and_dispatch_io(int timeout_millsec);
    int check_timers();
    void run_defers();
    void on_finish();
    int get_poll_timeout();

private:
    uint8_t _id;
    std::atomic_bool _is_running;
    std::atomic_bool _stopped;

    LoopCallback _on_loop_finish;

    RxEventPoller _event_poller;
    LoopCallback _on_timeout;
    EventHandler _handler_table[RxEventType::__Rx_EVENT_TYPE_COUNT][RxFDType::__RxFD_TYPE_COUNT];

    RxFD _event_fd;

    RxTimerHeap _timer_heap;

    RxMutex _defer_mutex;
    std::vector<DeferCallback> _defer_functors;

};

} //namespace Rinx
#endif // EVENTLOOP_H
