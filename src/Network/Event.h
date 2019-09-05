#ifndef EVENT_H
#define EVENT_H

#include <functional>

enum RxEventType:uint32_t{
    Rx_EVENT_READ=0,
    Rx_EVENT_WRITE,
    Rx_EVENT_ERROR,

    Rx_EVENT_TYPE_MAX
};

enum RxFDType:uint32_t{
    Rx_FD_LISTEN=0,
    Rx_FD_TCP_STREAM,
    Rx_FD_EVENT,
    Rx_FD_TYPE_MAX,
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

enum RxHandlerRes{
    Rx_HANDLER_OK=0,
    RX_HANDLER_ERR
};
using EventHandler=std::function<RxHandlerRes(const RxEvent &event_data)>;



#endif // EVENT_H
