#include "Server.h"
#include "Signal.h"
#include <iostream>
RxServer::RxServer(uint32_t max_connection):_max_connection(max_connection),_max_once_accept_count(128),
    _main_eventloop(0),_sub_eventloop_threads(std::thread::hardware_concurrency()),_connection_list(_max_connection)
{

}

bool RxServer::listen(const std::string &address, uint16_t port)
{
    signal_setup();

    RxListenPort ls_port(port);
    ls_port.serv_fd=RxSock::create_stream();

    if(!RxSock::is_open(ls_port.serv_fd)||!RxSock::set_nonblock(ls_port.serv_fd,true)){
        LOG_WARN<<"create serv sock failed";
        return false;
    }

    if(!RxSock::bind(ls_port.serv_fd,address.c_str(),ls_port.port)){
        LOG_WARN<<"bind address failed";
        RxSock::close(ls_port.serv_fd);
        return false;
    }

    if(!RxSock::listen(ls_port.serv_fd)){
        LOG_WARN<<"listen ["<<address<<":"<<ls_port.port<<"] failed";
        RxSock::close(ls_port.serv_fd);
        return false;
    }

    _listen_ports.emplace_back(std::make_pair(ls_port,std::make_unique<RxProtoProcHttp1Factory>()));
    g_threadpool::instantiate(4);

    if(!_main_eventloop.init()){
        LOG_WARN<<"main eventloop start failed";
        RxSock::close(ls_port.serv_fd);
        return false;
    }
    if(!_main_eventloop.monitor_fd_event(RxFD{Rx_FD_LISTEN,ls_port.serv_fd},{Rx_EVENT_READ,Rx_EVENT_WRITE})||
        !_main_eventloop.set_event_handler(Rx_FD_LISTEN,Rx_EVENT_READ,std::bind(&RxServer::on_acceptable,this,std::placeholders::_1)))
    {
        LOG_WARN<<"monitor serv sock failed, fd="<<ls_port.serv_fd;
        return false;
    }

    LOG_INFO<<"server listen on port "<<ls_port.port;

    _sub_eventloop_threads.for_each([this](RxThreadID,RxEventLoop *eventloop){
        eventloop->set_event_handler(Rx_FD_TCP_STREAM,Rx_EVENT_READ,std::bind(&RxServer::on_stream_readable,this,std::placeholders::_1));
        eventloop->set_event_handler(Rx_FD_TCP_STREAM,Rx_EVENT_ERROR,std::bind(&RxServer::on_stream_error,this,std::placeholders::_1));
    });
    _sub_eventloop_threads.start();

    /// only let the main eventloop thread handle signal asynchronously
    /// and let other threads block all signals
    _main_eventloop.set_loop_prepare([](RxEventLoop *){
        RxSignalManager::check_and_handle_async_signal();
    });

    RxSignalManager::enable_current_thread_signal();
//    std::thread t([this](){sleep(30); this->shutdown();});
    this->_main_eventloop.start_event_loop();
//    t.join();
    return true;
}

void RxServer::shutdown()
{
    std::cout<<"@shutdown"<<std::endl;
     _sub_eventloop_threads.shutdown();
    _main_eventloop.stop();
}

RxConnection *RxServer::incoming_connection(const RxFD rx_fd,RxEventLoop *eventloop)
{
    size_t conn_index=static_cast<size_t>(rx_fd.raw_fd);
    RxConnection *conn=&_connection_list[conn_index];
    conn->init(rx_fd,eventloop);
//    std::cout<<"@incoming conn fd="<<rx_fd.raw_fd<<std::endl;
    return conn;
}

RxConnection *RxServer::get_connection(const RxFD rx_fd)
{
    int fd=rx_fd.raw_fd;
    if(fd<0||fd>_max_connection){
        return nullptr;
    }
    return &_connection_list.at(static_cast<size_t>(rx_fd.raw_fd));
}


RxHandlerRc RxServer::on_acceptable(const RxEvent &event)
{
    for(int i=0;i<_max_once_accept_count;i++){
        RxAcceptRc accept_res;
        int new_fd=RxSock::accept(event.Fd.raw_fd,accept_res);

        switch (accept_res){
            case RxAcceptRc::ALL_ACCEPTED:
                return Rx_HANDLER_OK;

            case RxAcceptRc::FAILED:
                return Rx_HANDLER_OK;

            case RxAcceptRc::ERROR:
                LOG_WARN<<"Accept Error";
                disable_accept();
                return RX_HANDLER_ERR;

            default: break;
        }

        RxSock::set_nonblock(new_fd,true);

        RxFD client_rx_fd{Rx_FD_TCP_STREAM,new_fd};
        size_t sub_eventloop_index=new_fd%_sub_eventloop_threads.get_thread_num();
        RxEventLoop *sub_eventloop=_sub_eventloop_threads.get_eventloop(sub_eventloop_index);

        RxConnection *conn=this->incoming_connection(client_rx_fd,sub_eventloop);
        auto it_proto_proc_factory=std::find_if(_listen_ports.begin(),_listen_ports.end(),[&event](auto &ls_port){
            return ls_port.first.serv_fd==event.Fd.raw_fd;
        });

        assert(it_proto_proc_factory!=_listen_ports.end());
        std::unique_ptr<RxProtoProcessor> proto_processor=it_proto_proc_factory->second->new_proto_processor(*conn);

        conn->set_proto_processor(std::move(proto_processor));

        //TODO add it to incoming connection
        if(!sub_eventloop->monitor_fd_event(client_rx_fd,{Rx_EVENT_READ})){
            conn->close();
            LOG_WARN<<"watch fd "<<new_fd<<" failed";
            return RX_HANDLER_ERR;
        }

    }

    return Rx_HANDLER_OK;
}

void RxServer::disable_accept()
{

}

void RxServer::signal_setup()
{
    RxSignalManager::add_signal(SIGPIPE,&RxServer::signal_handler);
    RxSignalManager::add_signal(SIGINT,&RxServer::signal_handler);
}

void RxServer::signal_handler(const int signo)
{
    switch(signo) {
        case SIGINT:
            std::cout<<"server recv sig int, shutdown server..."<<std::endl;
        break;
        case SIGTERM:

        break;

        case SIGPIPE:
            std::cout<<"server recv SIGPIPE"<<std::endl;
        break;
        default:
            std::cout<<"server recv other sig"<<std::endl;
    }
}


RxHandlerRc RxServer::on_stream_readable(const RxEvent &event)
{
    RxConnection *conn=this->get_connection(event.Fd);
    if(!conn){
        ///TODO
        return RX_HANDLER_ERR;
    }

    RxReadRc read_rc;
    conn->recv(read_rc);

    switch(read_rc) {
        case RxReadRc::OK:
            conn->get_proto_processor().process_read_data(*conn,conn->get_input_buf());
            break;

        case RxReadRc::CLOSED:
            conn->close();
            break;

        case RxReadRc::ERROR:
            LOG_WARN<<"encounter error when read from fd "<<event.Fd.raw_fd
                    <<" errno="<<errno<<" description: "<<strerror(errno);
            conn->close();
            return Rx_HANDLER_EXIT_ALL;
    }

    return Rx_HANDLER_OK;
}

RxHandlerRc RxServer::on_stream_writable(const RxEvent &event)
{
    std::cout<<"@on stream writable"<<std::endl;
    RxConnection *conn=this->get_connection(event.Fd);
    if(!conn){
        ///TODO
        return RX_HANDLER_ERR;
    }

    conn->get_proto_processor().handle_write_prepared(*conn,conn->get_output_buf());


    ///TODO
//    RxWriteRc write_res;
//    conn->send(write_res);
//    if(write_res==RxWriteRc::ERROR){
//        conn->close();
//        return Rx_HANDLER_EXIT_ALL;
//    }
    return Rx_HANDLER_OK;
}

RxHandlerRc RxServer::on_stream_error(const RxEvent &event)
{
//    std::cout<<"@on_stream_error"<<std::endl;
    RxConnection *conn=this->get_connection(event.Fd);
    if(!conn){
        ///TODO
        return RX_HANDLER_ERR;
    }
    conn->close();
    return Rx_HANDLER_EXIT_ALL;
}
