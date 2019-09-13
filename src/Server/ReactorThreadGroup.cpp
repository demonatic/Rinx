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
    return index<_reactor_threads.size()?_reactor_threads[index].reactor.get():nullptr;
}


void RxReactorThreadGroup::reactor_for_each(std::function<void (RxReactor&)> f)
{
    for(auto &reactor_thread:_reactor_threads){
        if(reactor_thread.reactor){
            f(*reactor_thread.reactor);
        }
    }
}

void *RxReactorThreadGroup::reactor_thread_loop(void *reactor_obj){
    return (void*)((RxReactor*)(reactor_obj))->start_event_loop();
}
