#include "Rinx/Util/ThreadPool.h"

namespace Rinx {

template class RxThreadPool::Queue<RxThreadPool::Task>;

template<class T>
RxThreadPool::Queue<T>::Queue():_head(std::make_unique<QueueNode>()),_tail(_head.get())
{

}

template<class T>
void RxThreadPool::Queue<T>::push_back(T &&data)
{
    auto new_node=std::make_unique<QueueNode>();
    new_node->data=std::make_shared<T>(std::move(data));

    RxMutexLockGuard lock_guard(_tail_mutex);

    _tail->next=std::move(new_node);
    _tail=_tail->next.get();
}

template<class T>
std::optional<T> RxThreadPool::Queue<T>::try_pop()
{
    RxMutexLockGuard head_lock_guard(_head_mutex);
    {
        RxMutexLockGuard tail_lock_guard(_tail_mutex);
        //no data
        if(_head.get()==_tail){
            return std::nullopt;
        }
        //only one valid node
        if(_head->next.get()==_tail){
            _tail=_head.get();
            T data=*(_head->next->data);
            return std::make_optional<T>(data);
        }
    }

    T data=*(_head->next->data);
    _head->next=std::move(_head->next->next);
    return std::make_optional<T>(data);
}

template<class T>
bool RxThreadPool::Queue<T>::empty() const
{
    RxMutexLockGuard tail_lock_guard(_tail_mutex);
    return _tail==_head.get();
}


RxThreadPool::RxThreadPool (size_t thread_num):_stop(false),_index(0)
{
    for(size_t i=0;i<thread_num;i++){
        _workers.emplace_back(std::make_unique<ThreadWorker>());
    }
    this->start();
}

RxThreadPool::~RxThreadPool()
{
    if(!_stop){
        this->stop();
    }
}

void RxThreadPool::start()
{
    size_t worker_num=_workers.size();
    for(size_t i=0;i<worker_num;i++){
        _workers[i]->thread=std::thread([this,i,worker_num](){
            while(!_stop){
                try_get_one_task:
                for(size_t n=i;n<i+worker_num;n++){
                    size_t index=n%worker_num;
                    std::optional<Task> task=_workers[index]->task_queue.try_pop();
                    if(task.has_value()){
                        (*task)();
                        goto try_get_one_task;
                    }
                }

                //only go here when the worker find no work to do in the above loop
                ThreadWorker &self=*_workers[i];
                std::unique_lock<std::mutex> lock(self.idle_mutex);
                self.is_idle=true;
                self.condition_not_idle.wait(lock,
                    [&self,this]()->bool{return !self.task_queue.empty()||!self.is_idle||_stop;});
            }
       });
    }
}

void RxThreadPool::stop()
{
    _stop=true;
    for(auto &worker_ptr:_workers){
        if(worker_ptr->thread.joinable()){
            worker_ptr->condition_not_idle.notify_one();
            worker_ptr->thread.join();
        }
    }
}

} //namespace Rinx
