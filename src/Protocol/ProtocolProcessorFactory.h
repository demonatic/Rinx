#ifndef PROTOCOLPROCESSORFACTORY_H
#define PROTOCOLPROCESSORFACTORY_H

#include <memory>
#include "ProtocolProcessor.h"

class RxProtoProcFactory
{
public:
    RxProtoProcFactory();
    virtual std::unique_ptr<RxProtoProcessor> new_proto_processor(RxConnection &conn)=0;
    virtual ~RxProtoProcFactory();
};

#endif // PROTOCOLPROCESSORFACTORY_H
