#include "Connection.h"

RxConnection::RxConnection()
{

}

void RxConnection::init(const RxFD fd,RxReactor *reactor)
{
    _rx_fd=fd;
    _input_buf=RxChainBuffer::create_chain_buffer();
    _output_buf=RxChainBuffer::create_chain_buffer();
    _reactor_belongs=reactor;
}

ssize_t RxConnection::recv(Rx_Read_Res &read_res)
{
    return _input_buf->read_fd(_rx_fd.raw_fd,read_res);
}

ssize_t RxConnection::send(Rx_Write_Res &write_res)
{
    return _output_buf->write_fd(_rx_fd.raw_fd,write_res);
}

void RxConnection::close()
{
    _reactor_belongs->unmonitor_fd_event(_rx_fd);
    RxSock::close(_rx_fd.raw_fd);
    _input_buf.release();
    _output_buf.release();
}

RxProtoProcessor &RxConnection::get_proto_processor() const
{
    return *_proto_processor;
}

RxReactor &RxConnection::get_reactor() const
{
    return *_reactor_belongs;
}

RxFD RxConnection::get_rx_fd() const
{
    return _rx_fd;
}

void RxConnection::set_proto_processor(std::unique_ptr<RxProtoProcessor> &&processor) noexcept
{
    _proto_processor.swap(processor);
}

RxChainBuffer &RxConnection::get_input_buf()
{
    if(unlikely(_input_buf==nullptr)){
        _input_buf=RxChainBuffer::create_chain_buffer();
    }
    return *_input_buf;
}

RxChainBuffer &RxConnection::get_output_buf()
{
    if(unlikely(_output_buf==nullptr)){
        _output_buf=RxChainBuffer::create_chain_buffer();
    }
    return *_output_buf;
}
