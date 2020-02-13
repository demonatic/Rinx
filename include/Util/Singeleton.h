#ifndef SINGELETON_H
#define SINGELETON_H

#include <memory>
#include "Noncopyable.h"

namespace Rinx {

template<typename T>
class RxSingeleton:public RxNoncopyable
{
public:
    RxSingeleton()=delete;

    template<typename... Args>
    static T* instantiate(Args&& ...args){
        static T real_instance(std::forward<Args>(args)...);
        _instance=&real_instance;
        return _instance;
    }

    static T* get_instance(){
        if(!_instance){
            throw std::runtime_error("Singeleton must be instantiated before use");
        }
        return _instance;
    }

private:
    static T *_instance;
};

template<typename T> T* RxSingeleton<T>::_instance=nullptr;

} //namespace Rinx

#endif // SINGELETON_H
