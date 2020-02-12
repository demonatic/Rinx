#ifndef MUTEX_H
#define MUTEX_H

#include <pthread.h>
#include "Noncopyable.h"

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

#endif // MUTEX_H
