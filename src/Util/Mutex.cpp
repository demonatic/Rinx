#include "Mutex.h"

RxMutex::RxMutex()
{
    pthread_mutex_init(&_mutex,nullptr);
}

RxMutex::~RxMutex()
{
    pthread_mutex_destroy(&_mutex);
}

void RxMutex::lock()
{
    pthread_mutex_lock(&_mutex);
}

void RxMutex::unlock()
{
    pthread_mutex_unlock(&_mutex);
}

RxMutexLockGuard::RxMutexLockGuard(RxMutex &mutex):_mutex(mutex)
{
    _mutex.lock();
}

RxMutexLockGuard::~RxMutexLockGuard()
{
    _mutex.unlock();
}
