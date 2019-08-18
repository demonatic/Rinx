#ifndef EVENT_H
#define EVENT_H

#include <functional>

enum RxEventType:uint32_t{
    Rx_EVENT_READ=0,
    Rx_EVENT_WRITE,
    Rx_EVENT_ERROR,

    Rx_MAX_EVENT_TYPE
};

enum RxFDType:uint32_t{
    Rx_FD_LISTEN=0,
    Rx_FD_TCP_STREAM,

    Rx_MAX_FD_TYPE,
};

struct RxFD{
    RxFDType fd_type;
    int raw_fd;
};

class RxReactor;
struct RxEvent{
    RxFD Fd;
    RxReactor *reactor;
};

using EventHandler=std::function<int(const RxEvent &event_data)>;
enum class HandlerRes{
    Rx_HANDLER_OK,
    RX_HANDLER_ERR
};


#endif // EVENT_H
