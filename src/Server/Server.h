#ifndef SERVER_H
#define SERVER_H

#include <atomic>
#include <vector>
#include <list>
#include <string>
#include <assert.h>

#include "../Network/FD.h"
#include "../Network/Connection.h"
#include "../Network/EventLoop.h"
#include "../Protocol/ProtocolProcessorFactory.h"
#include "WorkerThreadLoops.h"
#include "../3rd/NanoLog/NanoLog.h"

struct RxListenPort{
    explicit RxListenPort(uint16_t port):port(port){
        serv_fd=RxFDHelper::Stream::create_serv_sock();
    }
    uint16_t port;
    RxFD serv_fd;
};

class RxServer
{
public:
    RxServer(uint32_t max_connection=65535);
    ~RxServer();

    template<typename ProtoFactory>
    bool listen(const std::string &address, uint16_t port,ProtoFactory factory)
    {
        std::cout<<"listen on port "<<port<<std::endl;
        if(!_start){
            LOG_WARN<<"Fail to listen because event loops start failed";
            return false;
        }

        RxListenPort ls_port(port);

        if(!RxFDHelper::is_open(ls_port.serv_fd)||!RxFDHelper::Stream::set_nonblock(ls_port.serv_fd,true)){
            LOG_WARN<<"create serv sock failed";
            return false;
        }

        if(!RxFDHelper::Stream::bind(ls_port.serv_fd,address.c_str(),ls_port.port)||!RxFDHelper::Stream::listen(ls_port.serv_fd)){
            LOG_WARN<<"bind or listen ["<<address<<":"<<ls_port.port<<"] failed:"<<errno<<' '<<strerror(errno);
            RxFDHelper::close(ls_port.serv_fd);
            return false;
        }

        _listen_ports.emplace_back(std::make_pair(ls_port,std::make_unique<ProtoFactory>(factory))); //TO OPTIMIZE COPY
        enable_accept();
//        _main_eventloop.register_fd(ls_port.serv_fd,{Rx_EVENT_READ,Rx_EVENT_WRITE,Rx_EVENT_ERROR});
//        LOG_INFO<<"listen on["<<address<<":"<<ls_port.port<<"]";


        RxSignalManager::enable_current_thread_signal();
        _main_eventloop.start_event_loop();
        return true;
    }

    void stop();

    void enable_accept();
    void disable_accept();

protected:
    bool on_acceptable(const RxEvent &event);
    bool on_stream_readable(const RxEvent &event);
    bool on_stream_writable(const RxEvent &event);

    /// @brief remote close, shutdown RW or RD, or send RST will lead to stream error
    bool on_stream_error(const RxEvent &event);

    void signal_setup();

    RxConnection *get_connection(const RxFD fd);

private:
    bool init_eventloops();

private:
    bool _start;
    std::once_flag _init_flag;

    uint32_t _max_connection;
    uint16_t _max_once_accept_count;
    uint16_t _eventloop_num;

    std::thread _acceptor_thread;
    RxEventLoop _main_eventloop;
    RxWorkerThreadLoops _sub_eventloop_threads;

    std::vector<RxConnection> _connection_list;
    std::list<std::pair<RxListenPort,std::unique_ptr<RxProtocolFactory>>> _listen_ports;

};

#endif // SERVER_H
