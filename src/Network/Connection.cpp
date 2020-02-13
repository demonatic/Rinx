#include "Network/Connection.h"
#include "3rd/NanoLog/NanoLog.h"
#include <iostream>

namespace Rinx {
RxConnection::RxConnection():_rx_fd(RxInvalidFD),_eventloop_belongs(nullptr),_sock_writable_flag(true)
{

}

RxConnection::~RxConnection()
{
    if(_rx_fd!=RxFDType::FD_INVALID){
        this->close();
    }
}

void RxConnection::init(const RxFD fd,RxEventLoop &eventloop)
{
    _rx_fd=fd;
    _input_buf=RxChainBuffer::create_chain_buffer();
    _output_buf=RxChainBuffer::create_chain_buffer();
    _eventloop_belongs=&eventloop;
}

bool RxConnection::activate()
{
    if(!_eventloop_belongs->monitor_fd(_rx_fd,{Rx_EVENT_READ,Rx_EVENT_WRITE,Rx_EVENT_ERROR})){
        LOG_WARN<<"monitor fd "<<_rx_fd.raw<<" failed";
        this->close();
        return false;
    }
    return true;
}

RxConnection::RecvRes RxConnection::recv()
{
    RecvRes res;
    do{
        auto n_read=_input_buf->read_from_fd(_rx_fd,res.code);
        if(n_read>0){
            res.recv_len+=n_read;
        }
    }while(res.code==RxReadRc::OK);
    return res;
}

RxConnection::SendRes RxConnection::send()
{
    SendRes res;
    do{
        auto n_sent=_output_buf->write_to_fd(_rx_fd,res.code);
        if(n_sent>0){
            res.send_len+=n_sent;
        }
    }
    while(!_output_buf->empty()&&res.code==RxWriteRc::OK);

    if(res.code==RxWriteRc::SOCK_SD_BUFF_FULL){
        _sock_writable_flag=false;
    }
    return res;
}

void RxConnection::close()
{
    if(_rx_fd!=RxInvalidFD){
        _eventloop_belongs->unmonitor_fd(_rx_fd);
        _input_buf.reset();
        _output_buf.reset();
        _eventloop_belongs=nullptr;
        _proto_processor.reset();
        //must put close at the end, or it would cause race condition
        std::cout<<"@RxConnection::close()"<<std::endl;
        if(!FDHelper::close(this->_rx_fd)){
            LOG_WARN<<"fd "<<_rx_fd.raw<<" close failed, errno "<<errno<<' '<<strerror(errno);
        }
    }
}

bool RxConnection::is_open() const
{
    return FDHelper::is_open(_rx_fd);
}

bool RxConnection::sock_writable() const
{
    return _sock_writable_flag;
}

void RxConnection::set_sock_to_writable()
{
    _sock_writable_flag=true;
}

RxProtoProcessor& RxConnection::get_proto_processor() const
{
    return *_proto_processor;
}

RxEventLoop* RxConnection::get_eventloop() const
{
    return _eventloop_belongs;
}

RxFD RxConnection::get_rx_fd() const
{
    return _rx_fd;
}

void RxConnection::set_proto_processor(std::unique_ptr<RxProtoProcessor> processor) noexcept
{
    _proto_processor.swap(processor);
}

RxChainBuffer& RxConnection::get_input_buf() const
{
    return *_input_buf;
}

RxChainBuffer& RxConnection::get_output_buf() const
{
    return *_output_buf;
}
} //namespace Rinx
