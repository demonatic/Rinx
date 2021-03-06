#ifndef SIGNAL_H
#define SIGNAL_H

#include <sys/signal.h>
#include <functional>
#include "Rinx/RinxDefines.h"

namespace Rinx {

using RxSignalHandler=std::function<void(int)>;
const RxSignalHandler RxSigHandlerIgnore=nullptr;

struct RxSignal
{
    bool setted=false;
    int signal_no=0;
    RxSignalHandler handler=nullptr;
};

class RxSignalManager{
public:
    RxSignalManager()=delete;
    static void on_signal(int _signo,RxSignalHandler handler);

    /// @brief block all signals of the caller thread
    static bool disable_current_thread_signal();
    static bool enable_current_thread_signal();

    static void check_and_handle_async_signal();
    static void trigger_signal(int _signo);
private:
    static void async_sig_handler(int _signo);

private:
    static volatile sig_atomic_t _signo;
    inline static constexpr size_t SignoMax=128;
    static RxSignal _signals[SignoMax];
};

} //namespace Rinx

#endif // SIGNAL_H
