#ifndef PROTOCOLHTTP1_H
#define PROTOCOLHTTP1_H

#include "../ProtocolProcessor.h"
#include "../ProtocolProcessorFactory.h"
#include "HttpParser.h"
#include "HttpReqRouter.h"

class RxProtoHttp1Processor:public RxProtoProcessor
{
public:
    RxProtoHttp1Processor();
    virtual ~RxProtoHttp1Processor();
    virtual void init(RxConnection &conn) override;

    virtual ProcessStatus process_read_data(RxConnection &conn,RxChainBuffer &input_buf) override;
    virtual ProcessStatus handle_write_prepared(RxConnection &conn,RxChainBuffer &output_buf) override;

private:
    HttpParser _request_parser;
};


class RxProtoProcHttp1Factory:public RxProtoProcFactory
{
public:
    RxProtoProcHttp1Factory()=default;
    virtual std::unique_ptr<RxProtoProcessor> new_proto_processor(RxConnection &conn) override;
};

#endif // PROTOCOLHTTP1_H
