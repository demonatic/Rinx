#ifndef ONCECALL_H
#define ONCECALL_H

#include <functional>
#include "Noncopyable.h"

class RxCallOnce{
public:
    using Task=std::function<void(void)>;
    RxCallOnce(const Task &on_constructed, const Task &on_deconstructed=nullptr){
        if(on_constructed){
            on_constructed();
        }
        on_deconstructed_=on_deconstructed;
    }

    RxCallOnce(const Task &on_constructed, const Task &&on_deconstructed){
        if(on_constructed){
            on_constructed();
        }
        on_deconstructed_=std::move(on_deconstructed);
    }

    ~RxCallOnce(){
        if(on_deconstructed_){
            on_deconstructed_();
        }
    }

private:
    RxCallOnce()=delete;
    RxCallOnce(const RxCallOnce&)=delete;
    Task on_deconstructed_;
};
#endif // ONCECALL_H
