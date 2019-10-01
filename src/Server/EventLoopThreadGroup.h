#ifndef REACTORTHREADS_H
#define REACTORTHREADS_H


#include <pthread.h>
#include <vector>
#include <memory>
#include "../Network/EventLoop.h"
#include "../3rd/NanoLog/NanoLog.h"

using RxThreadID=pthread_t;

struct RxEventLoopThread{
    RxThreadID thread_id;
    std::unique_ptr<RxEventLoop> eventloop;
};

class RxEventLoopThreadGroup
{
public:
    RxEventLoopThreadGroup(size_t num);
    bool start();
    void shutdown();

    size_t get_thread_num() const noexcept;
    RxEventLoop* get_eventloop(size_t index);

    void for_each(std::function<void(RxThreadID,RxEventLoop*)> f);

private:
    static void *eventloop_thread_loop(void *eventloop_obj);

private:
    std::vector<RxEventLoopThread> _eventloop_threads;
};

#endif // REACTORTHREADS_H
