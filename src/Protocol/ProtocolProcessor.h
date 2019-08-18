#ifndef SERVERPROTOCOL_H
#define SERVERPROTOCOL_H

#include "../Network/Buffer.h"
#include "HFSMParser.h"

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

    virtual void init()=0;
    virtual ProcessStatus process(RxConnection &conn,RxChainBuffer &buf)=0;
};


#endif // SERVERPROTOCOL_H
