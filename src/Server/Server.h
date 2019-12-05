#ifndef SERVER_H
#define SERVER_H

#include <atomic>
#include <vector>
#include <list>
#include <string>
#include <assert.h>
#include "../Network/Connection.h"
#include "../Network/EventLoop.h"
#include "../Network/Listener.h"
#include "../Protocol/ProtocolProcessorFactory.h"
#include "WorkerThreadLoops.h"

class RxServer
{
public:
    RxServer();
    ~RxServer();

    template<typename ProtoFactory>
    bool listen(const std::string &address, uint16_t port,ProtoFactory factory)
    {
        std::cout<<"try to listen on port "<<port<<std::endl;
        if(!_start){
            LOG_WARN<<"Fail to listen because server event loops init failed";
            return false;
        }
        try {
            RxListener listener(port);
            listener.bind(address,port);
            listener.listen();
            _listen_ports.emplace_back(std::make_pair(listener,std::make_unique<ProtoFactory>(std::move(factory))));

            enable_accept();
            RxSignalManager::enable_current_thread_signal();

            _main_eventloop.start_event_loop();

        }  catch (std::runtime_error &err){
            LOG_WARN<<"fail to start server, "<<err.what();
            return false;
        }
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

    uint32_t _max_connection;
    uint16_t _max_once_accept_count;
    uint16_t _eventloop_num;

    std::thread _acceptor_thread;
    RxEventLoop _main_eventloop;
    RxWorkerThreadLoops _sub_eventloop_threads;

    std::vector<RxConnection> _connection_list;
    std::list<std::pair<RxListener,std::unique_ptr<RxProtocolFactory>>> _listen_ports;

};

#endif // SERVER_H
