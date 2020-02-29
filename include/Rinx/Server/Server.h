#ifndef SERVER_H
#define SERVER_H

#include <atomic>
#include <vector>
#include <list>
#include <string>
#include "Rinx/Network/Connection.h"
#include "Rinx/Network/EventLoop.h"
#include "Rinx/Network/Listener.h"
#include "Rinx/Protocol/ProtocolProcessorFactory.h"
#include "WorkerThreadLoops.h"

namespace Rinx {

class RxServer:RxNoncopyable
{
public:
    RxServer();
    ~RxServer();

    template<typename ProtoFactory>
    bool listen(const std::string &address, uint16_t port,ProtoFactory factory)
    {
        nanolog::initialize(nanolog::GuaranteedLogger(),_log_dir.empty()?"/tmp/":_log_dir,"rinx_log",10);
        LOG_INFO<<"try binding on port "<<port;

        try {
            RxSignalManager::disable_current_thread_signal();

            if(!init_eventloops()){
                throw std::runtime_error("fail to init eventloops");
            }
            this->add_additional_protocol(address,port,factory);
            for(auto &[listener,proto_factory]:this->_listen_ports){
                listener.bind();
                listener.listen();
            }

            if(!_sub_eventloop_threads.start()){
                throw std::runtime_error("Fail to start worker eventloops");
            }

            signal_setup();
            RxSignalManager::enable_current_thread_signal();

            _start=true;
            enable_accept();
            _main_eventloop.start_event_loop(); //block here

        }  catch (std::runtime_error &err){
            LOG_CRIT<<"Fail to start server, reason: \""<<err.what()<<"\"";
            return false;
        }
        return true;
    }

    template<typename ProtoFactory>
    void add_additional_protocol(const std::string &address,const uint16_t port, ProtoFactory &factory){
        RxListener listener(port,address);
        _listen_ports.emplace_back(std::make_pair(listener,std::make_unique<ProtoFactory>(std::move(factory))));
    }

    void stop();
    bool is_running() const;

    void set_log_dir(const std::string &log_dir);

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
    std::atomic_bool _start;

    uint32_t _max_connection;
    uint16_t _max_once_accept_count;
    uint16_t _eventloop_num;

    std::string _log_dir;

    RxEventLoop _main_eventloop;
    RxWorkerThreadLoops _sub_eventloop_threads;

    std::vector<RxConnection> _connection_list;
    std::list<std::pair<RxListener,std::unique_ptr<RxProtocolFactory>>> _listen_ports;

};

} //namespace Rinx
#endif // SERVER_H
