#include "Server.h"
#include <iostream>
RxServer::RxServer(uint32_t max_connection):_max_connection(max_connection),_max_once_accept_count(128),
    _main_reactor(0),_sub_reactor_threads(4),_connection_list(_max_connection)
{

}

bool RxServer::start(const std::string &address, uint16_t port)
{
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

    if(!_main_reactor.init()){
        LOG_WARN<<"main reactor start failed";
        RxSock::close(ls_port.serv_fd);
        return false;
    }
    if(!_main_reactor.monitor_fd_event(RxFD{Rx_FD_LISTEN,ls_port.serv_fd},{Rx_EVENT_READ,Rx_EVENT_WRITE})||
            !_main_reactor.set_event_handler(Rx_FD_LISTEN,Rx_EVENT_READ,
                std::bind(&RxServer::on_acceptable,this,std::placeholders::_1)))
    {
        LOG_WARN<<"monitor serv sock failed, fd="<<ls_port.serv_fd;
        return false;
    }

    LOG_INFO<<"server listen on port "<<ls_port.port;

    _sub_reactor_threads.start();
    _sub_reactor_threads.set_each_reactor_handler(Rx_FD_TCP_STREAM,Rx_EVENT_READ,
        std::bind(&RxServer::on_tcp_readable,this,std::placeholders::_1));

    if(!_main_reactor.start_event_loop()){
        LOG_INFO<<"server stopped";
    }
    return true;
}

RxConnection *RxServer::incoming_connection(const RxFD rx_fd,RxReactor *reactor)
{
    size_t conn_index=static_cast<size_t>(rx_fd.raw_fd);
    RxConnection *conn=&_connection_list[conn_index];
    conn->init(rx_fd,reactor);
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

void RxServer::proto_handle(RxConnection &conn)
{
     conn.get_proto_processor().process(conn,conn.get_input_buf());
}

RxHandlerRes RxServer::on_acceptable(const RxEvent &event)
{
    assert(event.Fd.fd_type==Rx_FD_LISTEN);
    for(int i=0;i<_max_once_accept_count;i++){
        Rx_Accept_Res accept_res;

        int new_fd=RxSock::accept(event.Fd.raw_fd,accept_res);
        switch (accept_res){
            case Rx_Accept_Res::ALL_ACCEPTED:
                return Rx_HANDLER_OK;
            case Rx_Accept_Res::FAILED:
                return Rx_HANDLER_OK;
            case Rx_Accept_Res::ERROR:
                LOG_WARN<<"accept error";
                disable_accept();
                return RX_HANDLER_ERR;
            default: break;
        }

        RxSock::set_nonblock(new_fd,true);

        auto it_proto_proc_factory=std::find_if(_listen_ports.begin(),_listen_ports.end(),[&event](auto &ls_port){
            return ls_port.first.serv_fd==event.Fd.raw_fd;
        });

        assert(it_proto_proc_factory!=_listen_ports.end());
        std::unique_ptr<RxProtoProcessor> proto_processor=it_proto_proc_factory->second->new_proto_processor();

        RxFD client_rx_fd{Rx_FD_TCP_STREAM,new_fd};
        size_t sub_reactor_index=new_fd%_sub_reactor_threads.get_thread_num();
        RxReactor *sub_reactor=_sub_reactor_threads.get_reactor(sub_reactor_index);
        _reactor_round_index++;

        RxConnection *conn=this->incoming_connection(client_rx_fd,sub_reactor);
        conn->set_proto_processor(std::move(proto_processor));

        if(!sub_reactor->monitor_fd_event(client_rx_fd,{Rx_EVENT_READ})){
            RxSock::close(new_fd);
            LOG_WARN<<"watch fd "<<new_fd<<" failed";
            return RX_HANDLER_ERR;
        }
    }

    return Rx_HANDLER_OK;
}

void RxServer::disable_accept()
{

}


RxHandlerRes RxServer::on_tcp_readable(const RxEvent &event)
{
    RxConnection *conn=this->get_connection(event.Fd);

    Rx_Read_Res read_res;
    conn->recv(read_res);

    switch(read_res) {
        case Rx_Read_Res::OK:
            this->proto_handle(*conn);
            break;

        case Rx_Read_Res::ERROR:
            LOG_WARN<<"fd "<<event.Fd.raw_fd<<" encounter fatal error";
            conn->close();
            return RX_HANDLER_ERR;

        case Rx_Read_Res::CLOSED:
            LOG_INFO<<"connection close";
            conn->close();
            break;
    }

    return Rx_HANDLER_OK;
}

RxHandlerRes RxServer::on_tcp_writable(const RxEvent &event)
{
    RxConnection *conn=this->get_connection(event.Fd);
    assert(conn);

    Rx_Write_Res write_res;
    conn->send(write_res);
    if(write_res==Rx_Write_Res::ERROR){
        conn->close();
        return RX_HANDLER_ERR;
    }
    return Rx_HANDLER_OK;
}
