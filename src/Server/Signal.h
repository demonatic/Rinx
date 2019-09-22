#ifndef SIGNAL_H
#define SIGNAL_H

#include <sys/signal.h>
#include "../RinxDefines.h"

typedef void (*RxSignalHandler)(int);
const RxSignalHandler RxSigHandlerDefault=SIG_DFL;
const RxSignalHandler RxSigHandlerIgnore=SIG_IGN;

struct RxSignal
{
    bool setted=false;
    int signal_no=0;
    RxSignalHandler handler=nullptr;
};

class RxSignalManager{
public:
    RxSignalManager()=delete;
    static void add_signal(int _signo,RxSignalHandler handler);

    /// @brief block all signals of the caller thread
    static bool disable_current_thread_signal();
    static bool enable_current_thread_signal();

    static void check_and_handle_async_signal();
    static void trigger_signal(int _signo);
private:
    static void async_sig_handler(int _signo);

private:
    static volatile sig_atomic_t _signo;
    static RxSignal _signals[RX_SIGNO_MAX];


};



#endif // SIGNAL_H
