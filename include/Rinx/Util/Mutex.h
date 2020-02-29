#ifndef MUTEX_H
#define MUTEX_H

#include <pthread.h>
#include "Noncopyable.h"

namespace Rinx {

class RxMutex:public RxNoncopyable
{
public:
    RxMutex();
    ~RxMutex();
    void lock();
    void unlock();

private:
    pthread_mutex_t _mutex;
};

class RxMutexLockGuard:public RxNoncopyable{
public:
    explicit RxMutexLockGuard(RxMutex &mutex);
    ~RxMutexLockGuard();

private:
    RxMutex &_mutex;
};

} //namespace Rinx
#endif // MUTEX_H
