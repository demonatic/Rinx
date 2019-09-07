#ifndef SIGNAL_H
#define SIGNAL_H

#include <functional>
#include <array>
#include <sys/signal.h>
#include "../RinxDefines.h"

typedef void (*RxSignalHandler)(int);

struct RxSignal
{
    int signal_no;
    RxSignalHandler handler;
    bool is_active;
};

//To Implement
/// the idea is to only let the main reactor thread handle signal asynchronously
/// and let other threads ignore signals' handle
class RxReactor;
class RxSignalManager{
public:
    void bind_reactor(RxReactor *reactor);
    void set_signal(int signo,RxSignalHandler handler,int mask);

    void disable_current_thread_signal();

    int on_signal(int signo);

private:
    std::array<RxSignal,RX_SIGNO_MAX> signals;
};


#endif // SIGNAL_H
