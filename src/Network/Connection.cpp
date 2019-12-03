#include "Connection.h"
#include "../3rd/NanoLog/NanoLog.h"

RxConnection::RxConnection():_rx_fd(RxInvalidFD),_eventloop_belongs(nullptr)
{

}

RxConnection::~RxConnection()
{
    if(_rx_fd.raw!=-1){
        close();
    }
}

bool RxConnection::init(const RxFD fd,RxEventLoop &eventloop,RxProtocolFactory &factory)
{
    _rx_fd=fd;
    _input_buf=RxChainBuffer::create_chain_buffer();
    _output_buf=RxChainBuffer::create_chain_buffer();
    _eventloop_belongs=&eventloop;
    set_proto_processor(factory.new_proto_processor(this));

    if(!eventloop.register_fd(fd,{Rx_EVENT_READ,Rx_EVENT_WRITE,Rx_EVENT_ERROR})){
        this->close();
        LOG_WARN<<"monitor fd "<<fd.raw<<" failed";
        return false;
    }
    return true;
}

RxConnection::RecvRes RxConnection::recv()
{
    RecvRes res;
    res.recv_len=_input_buf->read_from_fd(_rx_fd,res.code);
    return res;
}

RxConnection::SendRes RxConnection::send()
{
    std::cout<<"@RxConn send"<<std::endl;
    SendRes res;
    res.send_len=_output_buf->write_to_fd(_rx_fd,res.code);
    return res;
}

void RxConnection::close()
{
    if(_rx_fd!=RxInvalidFD){
        _eventloop_belongs->unregister_fd(_rx_fd);
        _input_buf.reset();
        _output_buf.reset();
        _eventloop_belongs=nullptr;
        _proto_processor.reset();
        //must put close at the end, or it would cause race condition
        std::cout<<"@RxConnection::close()"<<std::endl;
        if(!RxFDHelper::close(this->_rx_fd)){
            LOG_WARN<<"fd "<<_rx_fd.raw<<" close failed, errno "<<errno<<' '<<strerror(errno);
        }
    }
}

bool RxConnection::is_open() const
{
    return RxFDHelper::is_open(_rx_fd);
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

void RxConnection::set_proto_processor(std::unique_ptr<RxProtoProcessor> &&processor) noexcept
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
