#include "Server/Signal.h"
#include "Network/EventLoop.h"

namespace Rinx {

RxSignal RxSignalManager::_signals[SignoMax];
volatile sig_atomic_t RxSignalManager::_signo=0;


void RxSignalManager::on_signal(int signo, RxSignalHandler handler)
{
    _signals[signo].signal_no=signo;
    _signals[signo].handler=handler;

    struct sigaction act,old_act;
    act.sa_handler=(handler==nullptr)?SIG_IGN:&async_sig_handler;
    sigemptyset(&act.sa_mask);
    act.sa_flags=0;
    if(sigaction(signo,&act,&old_act)>=0){
        _signals[signo].setted=true;
    }
}

bool RxSignalManager::disable_current_thread_signal()
{
    sigset_t mask;
    sigfillset(&mask);
    return pthread_sigmask(SIG_SETMASK,&mask,nullptr)>=0;
}

bool RxSignalManager::enable_current_thread_signal()
{
    sigset_t mask;
    sigfillset(&mask);
    return pthread_sigmask(SIG_UNBLOCK,&mask,nullptr)>=0;
}

void RxSignalManager::check_and_handle_async_signal()
{
    if(_signo){
        trigger_signal(_signo);
        _signo=0;
    }
}

void RxSignalManager::trigger_signal(int signo)
{
    if(_signals[signo].setted){
       RxSignalHandler sig_handler=_signals[signo].handler;
       if(sig_handler){
           sig_handler(signo);
       }
    }
}

void RxSignalManager::async_sig_handler(int signo)
{
    RxSignalManager::_signo=signo;
}

} //namespace Rinx
