#ifndef CONNECTION_H
#define CONNECTION_H

#include <memory>
#include <any>
#include "FD.h"
#include "Buffer.h"
#include "../Util/Util.h"
#include "../RinxDefines.h"
#include "../Protocol/ProtocolProcessorFactory.h"

class RxConnection:public RxNoncopyable
{
public:
    struct RecvRes{RxReadRc code; ssize_t recv_len;};
    struct SendRes{RxWriteRc code; ssize_t send_len;};
public:
    RxConnection();
    ~RxConnection();

    bool init(const RxFD fd,RxEventLoop &eventloop,RxProtocolFactory &factory);

    RecvRes recv();
    SendRes send();

    void close();
    bool is_open() const;

    RxChainBuffer& get_input_buf() const;
    RxChainBuffer& get_output_buf() const;

    RxProtoProcessor& get_proto_processor() const;
    RxEventLoop* get_eventloop() const;
    RxFD get_rx_fd() const;

    void set_proto_processor(std::unique_ptr<RxProtoProcessor> processor) noexcept;

private:
    RxFD _rx_fd;

    std::unique_ptr<RxChainBuffer> _input_buf;
    std::unique_ptr<RxChainBuffer> _output_buf;

    std::unique_ptr<RxProtoProcessor> _proto_processor;
    RxEventLoop *_eventloop_belongs;

};



#endif // CONNECTION_H
