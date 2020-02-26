#include "Server/WorkerThreadLoops.h"

namespace Rinx {

RxWorkerThreadLoops::RxWorkerThreadLoops(size_t num):_eventloop_threads(num)
{

}

bool RxWorkerThreadLoops::init()
{
    bool init_ok=true;
    size_t loop_id=0;
    for(RxEventLoopThread &loop_thread:_eventloop_threads){
        loop_thread.eventloop=std::make_unique<RxEventLoop>(++loop_id);
        if(!loop_thread.eventloop->init()){
            init_ok=false;
            break;
        }
    }
    return init_ok;
}

bool RxWorkerThreadLoops::start()
{
    for(auto &eventloop_thd:_eventloop_threads){
        pthread_t ptid;
        if(pthread_create(&ptid,nullptr,&RxWorkerThreadLoops::eventloop_thread_loop,eventloop_thd.eventloop.get())<0)
        {
            LOG_WARN<<"pthread create failed";
            return false;
        }
        eventloop_thd.thread_id=ptid;
    }
    return true;
}

void RxWorkerThreadLoops::stop()
{
    for_each([](RxThreadID tid,RxEventLoop *eventloop){
        eventloop->stop_event_loop();
        if(pthread_join(tid,nullptr)<0){
            LOG_WARN<<"pthread_join failed. thread_id="<<tid;
        }
    });
}

size_t RxWorkerThreadLoops::get_thread_num() const noexcept
{
    return _eventloop_threads.size();
}

RxEventLoop& RxWorkerThreadLoops::get_eventloop(size_t index)
{
    return *_eventloop_threads[index].eventloop;
}


void RxWorkerThreadLoops::for_each(std::function<void(RxThreadID,RxEventLoop*)> f)
{
    for(auto &eventloop_thread:_eventloop_threads){
        if(eventloop_thread.eventloop){
            f(eventloop_thread.thread_id,eventloop_thread.eventloop.get());
        }
    }
}

void *RxWorkerThreadLoops::eventloop_thread_loop(void *eventloop_obj){
    return (void*)((RxEventLoop*)(eventloop_obj))->start_event_loop();
}

} //namespace Rinx
