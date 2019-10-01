#include "EventLoopThreadGroup.h"

RxEventLoopThreadGroup::RxEventLoopThreadGroup(size_t num):_eventloop_threads(num)
{
    for(size_t i=0;i<_eventloop_threads.size();i++){
        RxEventLoopThread &eventloop_thd=_eventloop_threads[i];
        eventloop_thd.eventloop=std::make_unique<RxEventLoop>(i+1);
        eventloop_thd.eventloop->init();
    }
}

bool RxEventLoopThreadGroup::start()
{
    for(auto &eventloop_thd:_eventloop_threads){
        pthread_t ptid;
        if(pthread_create(&ptid,nullptr,&RxEventLoopThreadGroup::eventloop_thread_loop,eventloop_thd.eventloop.get())<0)
        {
            LOG_WARN<<"pthread create failed";
            return false;
        }
        eventloop_thd.thread_id=ptid;
    }
    return true;
}

void RxEventLoopThreadGroup::shutdown()
{
    for(RxEventLoopThread &eventloop_thd:_eventloop_threads){
        for_each([](RxThreadID,RxEventLoop *eventloop){
            eventloop->stop();
        });
        RxThreadID tid=eventloop_thd.thread_id;
        if(pthread_cancel(tid)<0){
            LOG_WARN<<"pthread_cancel failed. thread_id="<<tid;
        }
        if(pthread_join(tid,nullptr)<0){
            LOG_WARN<<"pthread_join failed. thread_id="<<tid;
        }
    }
}

size_t RxEventLoopThreadGroup::get_thread_num() const noexcept
{
    return _eventloop_threads.size();
}

RxEventLoop* RxEventLoopThreadGroup::get_eventloop(size_t index)
{
    return index<_eventloop_threads.size()?_eventloop_threads[index].eventloop.get():nullptr;
}


void RxEventLoopThreadGroup::for_each(std::function<void(RxThreadID,RxEventLoop*)> f)
{
    for(auto &eventloop_thread:_eventloop_threads){
        if(eventloop_thread.eventloop){
            f(eventloop_thread.thread_id,eventloop_thread.eventloop.get());
        }
    }
}

void *RxEventLoopThreadGroup::eventloop_thread_loop(void *eventloop_obj){
    return (void*)((RxEventLoop*)(eventloop_obj))->start_event_loop();
}
