#ifndef PROTOCOLHTTP1_H
#define PROTOCOLHTTP1_H

#include "../ProtocolProcessor.h"
#include "../ProtocolProcessorFactory.h"
#include "HttpParser.h"
#include "HttpPipeline.h"

class RxProtoHttp1Processor:public RxProtoProcessor
{
public:
    RxProtoHttp1Processor();
    virtual ~RxProtoHttp1Processor() override;
    virtual void init(RxConnection &conn) override;

    virtual ProcessStatus process_read_data(RxConnection &conn,RxChainBuffer &input_buf) override;
    virtual ProcessStatus handle_write_prepared(RxConnection &conn,RxChainBuffer &output_buf) override;

private:
    HttpParser _request_parser;
};


/// 1.处理完一个HttpRequest后再处理下一个HttpRequest
/// 2.当HttpParser发现一个HttpRequest到来后先截断剩余input_buffer内的数据，即每次最多提取出一个完整的HttpRequest，
/// 如果一次无法响应完则先不继续读input_buffer内的数据进行处理(EventLoop在socket有read事件时仍然会把数据append到input_buffer)，
/// 当connection.data字段（即HttpRequest）中的标志位表明request complete时才能重置request对象，继续处理下一个连接
class RxProtoProcHttp1Factory:public RxProtoProcFactory
{
public:
    RxProtoProcHttp1Factory()=default;
    virtual std::unique_ptr<RxProtoProcessor> new_proto_processor(RxConnection &conn) override;
};

#endif // PROTOCOLHTTP1_H
