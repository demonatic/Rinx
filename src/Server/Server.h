#ifndef SERVER_H
#define SERVER_H

#include <atomic>
#include <vector>
#include <list>
#include <string>
#include <assert.h>

#include "../Network/Socket.h"
#include "../Network/Connection.h"
#include "../Network/Reactor.h"
#include "../Protocol/HTTP/ProtocolHttp1.h"
#include "ReactorThreadGroup.h"
#include "../3rd/NanoLog/NanoLog.h"

struct RxListenPort{
    RxListenPort(uint16_t port):port(port){}

    uint16_t port;
    int serv_fd;
};


class RxServer
{
public:
    RxServer(uint32_t max_connection);
    bool start(const std::string &address,uint16_t port);
    bool destroy();

    RxConnection *incoming_connection(const RxFD rx_fd,RxReactor *reactor);
    RxConnection *get_connection(const RxFD rx_fd);

    void proto_handle(RxConnection &conn);

private:
    RxHandlerRes on_acceptable(const RxEvent &event);
    RxHandlerRes on_tcp_readable(const RxEvent &event);
    RxHandlerRes on_tcp_writable(const RxEvent &event);

    void disable_accept();

    static void signal_setup();
    static void signal_handler(const int signo);

private:
    uint32_t _max_connection;
    uint16_t _max_once_accept_count;
    uint16_t _reactor_num;

    uint16_t _reactor_round_index;

    RxReactor _main_reactor;
    RxReactorThreadGroup _sub_reactor_threads;

    std::vector<RxConnection> _connection_list;
    std::list<std::pair<RxListenPort,std::unique_ptr<RxProtoProcFactory>>> _listen_ports;
};

#endif // SERVER_H
