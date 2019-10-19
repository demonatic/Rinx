#ifndef SERVER_H
#define SERVER_H

#include <atomic>
#include <vector>
#include <list>
#include <string>
#include <assert.h>

#include "../Network/Socket.h"
#include "../Network/Connection.h"
#include "../Network/EventLoop.h"
#include "../Protocol/HTTP/ProtocolHttp1.h"
#include "WorkerThreadLoops.h"
#include "../3rd/NanoLog/NanoLog.h"

struct RxListenPort{
    RxListenPort(uint16_t port):port(port){}

    uint16_t port;
    int serv_fd;
};


class RxServer
{
public:
    RxServer(uint32_t max_connection=65535);
    bool listen(const std::string &address,uint16_t port);
    void shutdown();

    RxConnection *incoming_connection(const RxFD rx_fd,RxEventLoop *eventloop);
    RxConnection *get_connection(const RxFD rx_fd);

private:
    RxHandlerRc on_acceptable(const RxEvent &event);
    RxHandlerRc on_stream_readable(const RxEvent &event);
    RxHandlerRc on_stream_writable(const RxEvent &event);

    /// @brief remote close, shutdown RW or RD, or send RST will lead to stream error
    RxHandlerRc on_stream_error(const RxEvent &event);

    void disable_accept();

    static void signal_setup();
    static void signal_handler(const int signo);

private:
    uint32_t _max_connection;
    uint16_t _max_once_accept_count;
    uint16_t _eventloop_num;

    RxEventLoop _main_eventloop;
    RxWorkerThreadLoops _sub_eventloop_threads;

    std::vector<RxConnection> _connection_list;
    std::list<std::pair<RxListenPort,std::unique_ptr<RxProtoProcFactory>>> _listen_ports;
};

#endif // SERVER_H
