#ifndef CONNECTION_H
#define CONNECTION_H

#include <memory>
#include <any>
#include "Socket.h"
#include "Event.h"
#include "Buffer.h"
#include "../Util/Util.h"
#include "../RinxDefines.h"
#include "../Protocol/ProtocolProcessor.h"

class RxConnection:public RxNoncopyable
{
public:
    RxConnection();
    ~RxConnection();
    void init(const RxFD fd,RxEventLoop *eventloop);

    ssize_t recv(RxReadRc &read_res);
    ssize_t send(RxWriteRc &write_res);

    void close();

    RxChainBuffer& get_input_buf();
    RxChainBuffer& get_output_buf();

    RxProtoProcessor& get_proto_processor() const;
    RxEventLoop& get_eventloop() const;
    RxFD get_rx_fd() const;

    void set_proto_processor(std::unique_ptr<RxProtoProcessor> &&processor) noexcept;

public:
    std::any data;

private:
    RxFD _rx_fd;

    std::unique_ptr<RxChainBuffer> _input_buf;
    std::unique_ptr<RxChainBuffer> _output_buf;

    std::unique_ptr<RxProtoProcessor> _proto_processor;
    RxEventLoop *_eventloop_belongs;

};



#endif // CONNECTION_H
