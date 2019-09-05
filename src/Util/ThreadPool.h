#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <vector>
#include <memory>
#include <optional>
#include <functional>
#include <thread>
#include <future>
#include <atomic>
#include <iostream>
#include <assert.h>

#include "Mutex.h"

class RxThreadPool
{
    using Task=std::function<void()>;

    template<class T>
    class Queue{
        struct QueueNode{
            std::shared_ptr<T> data;
            std::unique_ptr<QueueNode> next;
        };
    public:
        Queue();
        bool empty() const;

        void push_back(T &&data);
        std::optional<T> try_pop();

    private:
        mutable RxMutex _head_mutex;
        std::unique_ptr<QueueNode> _head;

        mutable RxMutex _tail_mutex;
        QueueNode *_tail;
    };
    using TaskQueue=Queue<Task>;

public:

    RxThreadPool(size_t thread_num);
    ~RxThreadPool();

    void stop();

    template<typename F,typename ...Args>
    auto post(F &&f, Args&&... args)->std::future<typename std::invoke_result<F,Args...>::type>{
        if(_stop)
            throw std::runtime_error("post task on already stopped thread pool");

        using ret_type=typename std::invoke_result<F,Args...>::type;

        auto task_package=std::make_shared<std::packaged_task<ret_type()>>(
            std::bind(std::forward<F>(f),std::forward<Args>(args)...)
        );
        std::future<ret_type> async_res=task_package->get_future();

        ThreadWorker &worker=*_workers[_index++%_workers.size()];
        worker.task_queue.push_back([task_package](){
            (*task_package)();
        });

        if(worker.is_idle){
            std::lock_guard<std::mutex> lock(worker.idle_mutex);
            worker.is_idle=false;
            worker.condition_not_idle.notify_one();
        }

        return async_res;
    }

private:
    struct ThreadWorker{
        std::thread thread;
        TaskQueue task_queue;

        std::mutex idle_mutex;
        std::atomic_bool is_idle{true};
        std::condition_variable condition_not_idle;
    };

    std::atomic_bool _stop;

    std::atomic_size_t _index;
    std::vector<std::unique_ptr<ThreadWorker>> _workers;

};

#endif // THREADPOOL_H
