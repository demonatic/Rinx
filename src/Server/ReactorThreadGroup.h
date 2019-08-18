#ifndef REACTORTHREADS_H
#define REACTORTHREADS_H


#include <pthread.h>
#include <vector>
#include <memory>
#include "../Network/Reactor.h"
#include "../3rd/NanoLog/NanoLog.h"


struct RxReactorThread{
    pthread_t thread_id;
    std::unique_ptr<RxReactor> reactor;
};

class RxReactorThreadGroup
{
public:
    RxReactorThreadGroup(size_t num);
    bool start();
    void stop();

    size_t get_thread_num() const noexcept;
    RxReactor* get_reactor(size_t index);

    void set_each_reactor_handler(RxFDType fd_type,RxEventType event_type,EventHandler handler);

private:
    static void *reactor_thread_loop(void *reactor_obj);

private:
    std::vector<RxReactorThread> _reactor_threads;
};

#endif // REACTORTHREADS_H
