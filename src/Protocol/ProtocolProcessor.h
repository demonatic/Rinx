#ifndef SERVERPROTOCOL_H
#define SERVERPROTOCOL_H

#include "../Network/Buffer.h"
#include "../Network/EventLoop.h"
#include "HFSMParser.hpp"

class RxConnection;

enum class ProcessStatus{
    OK=0,
    Proto_Parse_Error,
    Request_Incoming,
    Request_Handler_Error,
};

class RxProtoProcessor
{
public:
    RxProtoProcessor()=default;
    virtual ~RxProtoProcessor()=default;

    virtual void init(RxConnection &conn)=0;

    virtual ProcessStatus process_read_data(RxConnection &conn,RxChainBuffer &input_buf)=0;
    virtual ProcessStatus handle_write_prepared(RxConnection &conn,RxChainBuffer &output_buf)=0;
};


#endif // SERVERPROTOCOL_H
