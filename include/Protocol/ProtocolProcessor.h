#ifndef SERVERPROTOCOL_H
#define SERVERPROTOCOL_H

#include "Network/Buffer.h"
#include "Network/EventLoop.h"
#include "HFSMParser.hpp"
#include "Util/Noncopyable.h"

namespace Rinx {

class RxConnection;

class RxProtoProcessor:RxNoncopyable
{
public:
    RxProtoProcessor(RxConnection *conn):_conn_belongs(conn){}
    virtual ~RxProtoProcessor();

    RxConnection *get_connection();

    /// @brief called by server when connection has successfully received some data in inputbuf
    /// @return true if successfully handle the incoming data
    virtual bool process_read_data(RxConnection *conn,RxChainBuffer &input_buf)=0;

    /// @brief called by server when the remaining data in outputbuf can be sent to socket
    /// @return true if successfully execute this handler function
    virtual bool handle_write_prepared(RxConnection *conn,RxChainBuffer &output_buf)=0;

protected:
    RxConnection *_conn_belongs;
};

} //namespace Rinx
#endif // SERVERPROTOCOL_H
