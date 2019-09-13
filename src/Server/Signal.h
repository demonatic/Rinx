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

//To Implement
/// the idea is to only let the main reactor thread handle signal asynchronously
/// and let other threads ignore signals' handle
class RxReactor;
class RxSignalManager{
public:
    static volatile sig_atomic_t signo;

public:
    static void add_signal(int signo,RxSignalHandler handler);

    /// @brief block all signals of the caller thread
    static bool disable_current_thread_signal();
    static bool enable_current_thread_signal();

    static void trigger_signal(int signo);
private:
    static void async_sig_handler(int signo);

private:
    static RxSignal _signals[RX_SIGNO_MAX];

};



#endif // SIGNAL_H
