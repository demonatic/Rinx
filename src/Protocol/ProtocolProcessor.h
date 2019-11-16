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
    RxProtoProcessor(RxConnection *conn):_conn_belongs(conn){}
    virtual ~RxProtoProcessor();

    RxConnection *conn_belongs();

    virtual ProcessStatus process_read_data(RxConnection *conn,RxChainBuffer &input_buf)=0;
    virtual ProcessStatus handle_write_prepared(RxConnection *conn,RxChainBuffer &output_buf)=0;

protected:
    RxConnection *_conn_belongs;
};


#endif // SERVERPROTOCOL_H
