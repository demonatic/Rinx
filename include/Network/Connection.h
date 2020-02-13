#ifndef CONNECTION_H
#define CONNECTION_H

#include <memory>
#include <any>
#include "FD.h"
#include "Buffer.h"
#include "Util/Util.h"
#include "RinxDefines.h"
#include "Protocol/ProtocolProcessorFactory.h"

namespace Rinx {

class RxConnection:public RxNoncopyable
{
public:
    struct RecvRes{RxReadRc code; ssize_t recv_len=0;};
    struct SendRes{RxWriteRc code; ssize_t send_len=0;};
public:
    RxConnection();
    ~RxConnection();

    void init(const RxFD fd,RxEventLoop &eventloop);
    bool activate();

    /// @brief read as much data as possible in input_buf
    /// @return the read status and the number of bytes been read
    RecvRes recv();
    /// @brief write as much data as possible from output_buf
    /// @return the write status and the number of bytes been written
    /// either all data is sent or socket send buffer is full and status be SOCK_SD_BUFF_FULL
    SendRes send();

    void close();
    bool is_open() const;

    /// @brief test whether socket write buffer is writable, if not, set it to be writable
    bool sock_writable() const;
    void set_sock_to_writable();

    RxFD get_rx_fd() const;
    RxEventLoop* get_eventloop() const;

    RxChainBuffer& get_input_buf() const;
    RxChainBuffer& get_output_buf() const;

    RxProtoProcessor& get_proto_processor() const;
    void set_proto_processor(std::unique_ptr<RxProtoProcessor> processor) noexcept;

private:
    RxFD _rx_fd;

    std::unique_ptr<RxChainBuffer> _input_buf;
    std::unique_ptr<RxChainBuffer> _output_buf;

    std::unique_ptr<RxProtoProcessor> _proto_processor;
    RxEventLoop *_eventloop_belongs;

    bool _sock_writable_flag; //we assume socket buffer to be writable until write return EAGAIN
};

} //namespace Rinx

#endif // CONNECTION_H
