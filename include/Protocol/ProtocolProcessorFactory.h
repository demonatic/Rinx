#ifndef PROTOCOLPROCESSORFACTORY_H
#define PROTOCOLPROCESSORFACTORY_H

#include <memory>
#include "ProtocolProcessor.h"

class RxProtocolFactory
{
public:
    RxProtocolFactory();
    virtual std::unique_ptr<RxProtoProcessor> new_proto_processor(RxConnection *conn)=0;
    virtual ~RxProtocolFactory();
};

#endif // PROTOCOLPROCESSORFACTORY_H
