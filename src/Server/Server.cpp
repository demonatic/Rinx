#include "Server.h"
#include "Signal.h"
#include <iostream>
RxServer::RxServer():_start(false),_max_connection(MaxConnectionNum),_max_once_accept_count(128),
    _main_eventloop(0),_sub_eventloop_threads(IOWorkerNum),_connection_list(_max_connection)
{
   _start=this->init_eventloops();
}

RxServer::~RxServer()
{
    stop();
}

void RxServer::stop()
{
    if(_start){
//        disable_accept();
        _sub_eventloop_threads.stop();
        _main_eventloop.stop_event_loop();
        _start=false;
    }
}

void RxServer::enable_accept()
{
    for(auto &pair:_listen_ports){
        RxListener &listen_port=pair.first;
        _main_eventloop.register_fd(listen_port.serv_fd,{Rx_EVENT_READ,Rx_EVENT_WRITE,Rx_EVENT_ERROR});
        LOG_INFO<<"server enable listen on port "<<listen_port.port;
    }
}

void RxServer::disable_accept()
{
    for(auto &pair:_listen_ports){
        RxListener &listen_port=pair.first;
        _main_eventloop.unregister_fd(listen_port.serv_fd);
        LOG_INFO<<"server disable listen on port "<<listen_port.port;
    }
}

RxConnection *RxServer::get_connection(const RxFD fd)
{
    int index=fd.raw;
    if(index<0||index>=_max_connection){
        return nullptr;
    }
    return &_connection_list[index];
}

bool RxServer::init_eventloops()
{
    RxSignalManager::disable_current_thread_signal();
    signal_setup();
    g_threadpool::instantiate(ThreadPoolWorkerNum);

    if(!_main_eventloop.init()){
        LOG_WARN<<"Fail to start main eventloop";
        return false;
    }
    _main_eventloop.set_event_handler(FD_LISTEN,Rx_EVENT_READ,std::bind(&RxServer::on_acceptable,this,std::placeholders::_1));

    _sub_eventloop_threads.for_each([this](RxThreadID,RxEventLoop *eventloop){
        eventloop->set_event_handler(FD_CLIENT_STREAM,Rx_EVENT_READ,std::bind(&RxServer::on_stream_readable,this,std::placeholders::_1));
        eventloop->set_event_handler(FD_CLIENT_STREAM,Rx_EVENT_ERROR,std::bind(&RxServer::on_stream_error,this,std::placeholders::_1));
    });
    if(!_sub_eventloop_threads.start()){
        LOG_WARN<<"Fail to start sub eventloops";
        return false;
    }

    /// only let the main eventloop thread handle signal asynchronously
    /// and let other threads block all signals
    _main_eventloop.set_loop_prepare([](RxEventLoop *){
        RxSignalManager::check_and_handle_async_signal();
    });
    return true;
}


bool RxServer::on_acceptable(const RxEvent &event)
{
    for(int i=0;i<_max_once_accept_count;i++){
        RxAcceptRc accept_res;
        RxFD client_fd=RxFDHelper::Stream::accept(event.Fd,accept_res);

        switch (accept_res){
            case RxAcceptRc::ALL_ACCEPTED:
                return true;

            case RxAcceptRc::FAILED:
                return false;

            case RxAcceptRc::ERROR:
                LOG_WARN<<"Accept Error:"<<errno<<' '<<strerror(errno);
                disable_accept();
                return false;

            case RxAcceptRc::ACCEPTING:
            break;
        }

        RxFDHelper::Stream::set_nonblock(client_fd,true);
        RxFDHelper::Stream::set_tcp_nodelay(client_fd,true);

        RxConnection *conn=this->get_connection(client_fd);
        //client connection reaches maximum  TODO disable accept?
        if(conn==nullptr){
            return false;
        }

        size_t worker_index=client_fd.raw%_sub_eventloop_threads.get_thread_num();
        RxEventLoop &sub_eventloop=_sub_eventloop_threads.get_eventloop(worker_index);

        auto it_proto_proc_factory=std::find_if(_listen_ports.begin(),_listen_ports.end(),[&event](auto &ls_port){
            return ls_port.first.serv_fd==event.Fd;
        });
        conn->init(client_fd,sub_eventloop,*it_proto_proc_factory->second);
    }

    return true;
}

void RxServer::signal_setup()
{
    RxSignalManager::on_signal(SIGPIPE,RxSigHandlerIgnore);
    RxSignalManager::on_signal(SIGBUS,[](int){ // for mmap

    });
    RxSignalManager::on_signal(SIGINT,[this](int signo){
        std::cout<<"shutdown server..."<<std::endl;
        this->stop();
    });
}

bool RxServer::on_stream_readable(const RxEvent &event)
{
    RxConnection *conn=this->get_connection(event.Fd);
    auto read_res=conn->recv();

    switch(read_res.code) {
        case RxReadRc::OK:
            if(!conn->get_proto_processor().process_read_data(conn,conn->get_input_buf())){
//                assert(false);
                conn->close();
                return false;
            }
            return true;

        case RxReadRc::ERROR:
            LOG_WARN<<"encounter error when read from fd "<<event.Fd.raw
                    <<" errno="<<errno<<" description: "<<strerror(errno);
             assert(false);
            conn->close();
            return false;

        case RxReadRc::CLOSED:
            std::cout<<"---------------------------remote closed----"<<std::endl;
            conn->close();
            return false;
    }
}

bool RxServer::on_stream_writable(const RxEvent &event)
{
    std::cout<<"@on stream writable"<<std::endl;
    RxConnection *conn=this->get_connection(event.Fd);
    return conn->get_proto_processor().handle_write_prepared(conn,conn->get_output_buf());
}

bool RxServer::on_stream_error(const RxEvent &event)
{
//    std::cout<<"@on_stream_error"<<std::endl;
    this->get_connection(event.Fd)->close();
    return false;
}
