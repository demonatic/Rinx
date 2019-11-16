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

void RxConnection::init(const RxFD fd,RxEventLoop *eventloop)
{
    _rx_fd=fd;
    _input_buf=RxChainBuffer::create_chain_buffer();
    _output_buf=RxChainBuffer::create_chain_buffer();
    _eventloop_belongs=eventloop;
}

ssize_t RxConnection::recv(RxReadRc &read_res)
{
    if(unlikely(!_input_buf)){
        read_res=RxReadRc::ERROR;
        LOG_WARN<<"try to read to an empty input buffer";
        return -1;
    }
    return _input_buf->read_from_fd(_rx_fd,read_res);
}

ssize_t RxConnection::send(RxWriteRc &write_res)
{
    if(unlikely(!_output_buf)){
        write_res=RxWriteRc::ERROR;
        LOG_WARN<<"try to read from an empty output buffer";
        return -1;
    }
    return _output_buf->write_to_fd(_rx_fd,write_res);
}

void RxConnection::close()
{
    if(_rx_fd!=RxInvalidFD){
        _eventloop_belongs->unmonitor_fd_event(_rx_fd);
        _input_buf.reset();
        _output_buf.reset();
        _proto_processor.reset();
        _eventloop_belongs=nullptr;
        data.reset();
        //must put close at the end, or it would cause race condition
        RxFDHelper::close(_rx_fd);
    }
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

RxChainBuffer& RxConnection::get_input_buf()
{
    return *_input_buf;
}

RxChainBuffer& RxConnection::get_output_buf()
{
    return *_output_buf;
}
