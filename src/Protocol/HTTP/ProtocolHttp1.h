#ifndef PROTOCOLHTTP1_H
#define PROTOCOLHTTP1_H

#include "../ProtocolProcessor.h"
#include "../ProtocolProcessorFactory.h"
#include "HttpParser.h"
#include "HttpReqHandlerEngine.h"

class RxProtoHttp1Processor:public RxProtoProcessor
{
public:
    RxProtoHttp1Processor();
    virtual void init() override;
    virtual ProcessStatus process(RxConnection &conn, RxChainBuffer &buf) override;

private:
    HttpParser _request_parser;
    static HttpReqHandlerEngine s_req_handler_engine;
};


class RxProtoProcHttp1Factory:public RxProtoProcFactory
{
public:
    RxProtoProcHttp1Factory(){}
    virtual std::unique_ptr<RxProtoProcessor> new_proto_processor() override;
};

#endif // PROTOCOLHTTP1_H
