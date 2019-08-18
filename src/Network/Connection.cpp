#include "Connection.h"

RxConnection::RxConnection()
{

}

void RxConnection::init(const RxFD fd) noexcept
{
    _rx_fd=fd;
    _input_buf=RxChainBuffer::create_chain_buffer();
}

ssize_t RxConnection::recv(Rx_Read_Res &read_res)
{
    return _input_buf->read_fd(_rx_fd.raw_fd,read_res);
}

ssize_t RxConnection::send(Rx_Write_Res &write_res)
{
    return _input_buf->write_fd(_rx_fd.raw_fd,write_res);
}

void RxConnection::close()
{
    RxSock::close(_rx_fd.raw_fd);
    _input_buf.release();
    _output_buf.release();
}

RxProtoProcessor &RxConnection::get_proto_processor() const
{
    return *_proto_processor;
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
