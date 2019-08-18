#include "ReactorThreadGroup.h"

RxReactorThreadGroup::RxReactorThreadGroup(size_t num):_reactor_threads(num)
{

}

bool RxReactorThreadGroup::start()
{
    for(size_t i=0;i<_reactor_threads.size();i++){
        RxReactorThread &reactor_thd=_reactor_threads[i];
        reactor_thd.reactor=std::make_unique<RxReactor>(i+1);
        reactor_thd.reactor->init();

        pthread_t ptid;
        if(pthread_create(&ptid,nullptr,&RxReactorThreadGroup::reactor_thread_loop,reactor_thd.reactor.get())<0)
        {
            LOG_WARN<<"pthread create failed";
            return false;
        }
        reactor_thd.thread_id=ptid;
    }
    return true;
}

void RxReactorThreadGroup::stop()
{
    for(RxReactorThread &reactor_thd:_reactor_threads){
        pthread_t ptid=reactor_thd.thread_id;
        if(pthread_cancel(ptid)<0){
            LOG_WARN<<"pthread_cancel failed. thread_id="<<ptid;
        }
        if(pthread_join(ptid,nullptr)<0){
            LOG_WARN<<"pthread_join failed. thread_id="<<ptid;
        }
    }
}

size_t RxReactorThreadGroup::get_thread_num() const noexcept
{
    return _reactor_threads.size();
}

RxReactor* RxReactorThreadGroup::get_reactor(size_t index)
{
    return _reactor_threads[index].reactor.get();
}

void RxReactorThreadGroup::set_each_reactor_handler(RxFDType fd_type, RxEventType event_type, EventHandler handler)
{
    for(auto &reactor_thd:_reactor_threads){
        if(reactor_thd.reactor){
            reactor_thd.reactor->set_event_handler(fd_type,event_type,handler);
        }
    }
}

void *RxReactorThreadGroup::reactor_thread_loop(void *reactor_obj){
    return (void*)((RxReactor*)(reactor_obj))->start_event_loop();
}
