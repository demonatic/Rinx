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

    if(!RxFDHelper::is_open(ls_port.serv_fd)||!RxFDHelper::Stream::set_nonblock(ls_port.serv_fd,true)){
        LOG_WARN<<"create serv sock failed";
        return false;
    }

    if(!RxFDHelper::Stream::bind(ls_port.serv_fd,address.c_str(),ls_port.port)){
        LOG_WARN<<"bind address failed";
        RxFDHelper::close(ls_port.serv_fd);
        return false;
    }

    if(!RxFDHelper::Stream::listen(ls_port.serv_fd)){
        LOG_WARN<<"listen ["<<address<<":"<<ls_port.port<<"] failed";
        RxFDHelper::close(ls_port.serv_fd);
        return false;
    }

    _listen_ports.emplace_back(std::make_pair(ls_port,std::make_unique<RxProtoProcHttp1Factory>())); //TODO
    g_threadpool::instantiate(4);

    if(!_main_eventloop.init()){
        LOG_WARN<<"main eventloop start failed";
        RxFDHelper::close(ls_port.serv_fd);
        return false;
    }
    if(!_main_eventloop.monitor_fd_event(ls_port.serv_fd,{Rx_EVENT_READ,Rx_EVENT_WRITE})||
        !_main_eventloop.set_event_handler(RxFD_LISTEN,Rx_EVENT_READ,std::bind(&RxServer::on_acceptable,this,std::placeholders::_1)))
    {
        LOG_WARN<<"monitor serv sock failed, fd="<<ls_port.serv_fd.raw;
        return false;
    }

    LOG_INFO<<"server listen on port "<<ls_port.port;

    _sub_eventloop_threads.for_each([this](RxThreadID,RxEventLoop *eventloop){
        eventloop->set_event_handler(RxFD_CLIENT_STREAM,Rx_EVENT_READ,std::bind(&RxServer::on_stream_readable,this,std::placeholders::_1));
        eventloop->set_event_handler(RxFD_CLIENT_STREAM,Rx_EVENT_ERROR,std::bind(&RxServer::on_stream_error,this,std::placeholders::_1));
    });
    _sub_eventloop_threads.start();

    /// only let the main eventloop thread handle signal asynchronously
    /// and let other threads block all signals
    _main_eventloop.set_loop_prepare([](RxEventLoop *){
        RxSignalManager::check_and_handle_async_signal();
    });

    RxSignalManager::enable_current_thread_signal();

    this->_main_eventloop.start_event_loop();
    return true;
}

void RxServer::stop()
{
    _main_eventloop.stop_event_loop();
    _sub_eventloop_threads.stop();
}

RxConnection *RxServer::incoming_connection(const RxFD rx_fd,RxEventLoop *eventloop)
{
//    std::cout<<"@incoming conn fd="<<rx_fd.raw<<std::endl;
    RxConnection *conn=this->get_connection(rx_fd);
    if(conn){
        conn->init(rx_fd,eventloop);
    }
    return conn;
}

RxConnection *RxServer::get_connection(const RxFD fd)
{
    int index=fd.raw;
    if(index<0||index>=_max_connection){
        return nullptr;
    }
    return &_connection_list[index];
}


RxHandlerRc RxServer::on_acceptable(const RxEvent &event)
{
    for(int i=0;i<_max_once_accept_count;i++){
        RxAcceptRc accept_res;
        RxFD client_fd=RxFDHelper::Stream::accept(event.Fd,accept_res);

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

        RxFDHelper::Stream::set_nonblock(client_fd,true);

        size_t sub_eventloop_index=client_fd.raw%_sub_eventloop_threads.get_thread_num();
        RxEventLoop *sub_eventloop=_sub_eventloop_threads.get_eventloop(sub_eventloop_index);

        RxConnection *conn=this->incoming_connection(client_fd,sub_eventloop);
        auto it_proto_proc_factory=std::find_if(_listen_ports.begin(),_listen_ports.end(),[&event](auto &ls_port){
            return ls_port.first.serv_fd==event.Fd;
        });

        std::unique_ptr<RxProtoProcessor> proto_processor=it_proto_proc_factory->second->new_proto_processor(conn);

        conn->set_proto_processor(std::move(proto_processor));

        //TODO add it to incoming connection
        if(!sub_eventloop->monitor_fd_event(client_fd,{Rx_EVENT_READ})){
            conn->close();
            LOG_WARN<<"watch fd "<<client_fd.raw<<" failed";
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
    RxSignalManager::on_signal(SIGPIPE,RxSigHandlerIgnore);
    RxSignalManager::on_signal(SIGTERM,[](int){

    });
    RxSignalManager::on_signal(SIGINT,[this](int signo){
        std::cout<<"server recv sig int (signal num="<<signo<<"), shutdown server..."<<std::endl;
        this->stop();
    });
}

RxHandlerRc RxServer::on_stream_readable(const RxEvent &event)
{
    RxConnection *conn=this->get_connection(event.Fd);
    if(!conn){
        ///TODO
        return RX_HANDLER_ERR;
    }

    RxReadRc read_rc;
    ssize_t read_n=conn->recv(read_rc);
//    std::cout<<"@on server readable: read"<<read_n<<std::endl;

    switch(read_rc) {
        case RxReadRc::OK:
            conn->get_proto_processor().process_read_data(conn,conn->get_input_buf());
            break;

        case RxReadRc::CLOSED:
            conn->close();
            break;

        case RxReadRc::ERROR:
            LOG_WARN<<"encounter error when read from fd "<<event.Fd.raw
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

    conn->get_proto_processor().handle_write_prepared(conn,conn->get_output_buf());
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
