#ifndef SERVERPROTOCOL_H
#define SERVERPROTOCOL_H

#include "../Network/Buffer.h"
#include "../Network/EventLoop.h"
#include "HFSMParser.hpp"

class RxConnection;

class RxProtoProcessor
{
public:
    RxProtoProcessor(RxConnection *conn):_conn_belongs(conn){}
    virtual ~RxProtoProcessor();

    RxConnection *get_connection();

    virtual bool process_read_data(RxConnection *conn,RxChainBuffer &input_buf)=0;
    virtual bool handle_write_prepared(RxConnection *conn,RxChainBuffer &output_buf)=0;

protected:
    RxConnection *_conn_belongs;
};


#endif // SERVERPROTOCOL_H
