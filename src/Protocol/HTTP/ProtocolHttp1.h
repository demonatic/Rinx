#ifndef PROTOCOLHTTP1_H
#define PROTOCOLHTTP1_H

#include "../ProtocolProcessor.h"
#include "../ProtocolProcessorFactory.h"
#include "HttpParser.h"
#include "HttpPipeline.h"

using HttpParse::HttpParser;

class RxProtoHttp1Processor:public RxProtoProcessor
{
public:
    RxProtoHttp1Processor(RxConnection *conn);
    virtual ~RxProtoHttp1Processor() override;

    virtual ProcessStatus process_read_data(RxConnection *conn,RxChainBuffer &input_buf) override;
    virtual ProcessStatus handle_write_prepared(RxConnection *conn,RxChainBuffer &output_buf) override;

protected:
    using InputDataRange=HttpParser::InputDataRange<RxChainBuffer::read_iterator>;

    /// callback when get http header on socket stream
    void on_header_recv(InputDataRange range,HttpRequest *http_request);

    /// callback when get part of http body on socket stream
    void on_part_of_body(InputDataRange range,HttpRequest *http_request);

    /// callback when a complete request has received
    void on_request_recv(InputDataRange range,HttpRequest *http_request);

    /// callback when an http protocol parse error occurs
    void on_parse_error(InputDataRange range,HttpRequest *http_request);

private:
    void set_timeout(uint64_t sec,TimerCallback timeout_cb=nullptr){
        if(!timeout_cb){
            timeout_cb=[](){printf("time out");}; //TODO
        }
        cancel_read_timer();
        _read_timer->start_timer(sec*1000,timeout_cb);
    }

    void cancel_read_timer(){
        if(_read_timer->is_active()){
            _read_timer->stop();
        }
    }

private:
    HttpParser _request_parser;
    std::unique_ptr<RxTimer> _read_timer;
};


/// 1.处理完一个HttpRequest后再处理下一个HttpRequest
/// 2.当HttpParser发现一个HttpRequest到来后先截断剩余input_buffer内的数据，即每次最多提取出一个完整的HttpRequest，
/// 如果一次无法响应完则先不继续读input_buffer内的数据进行处理(EventLoop在socket有read事件时仍然会把数据append到input_buffer)，
/// 当connection.data字段（即HttpRequest）中的标志位表明request complete时才能重置request对象，继续处理下一个连接
class RxProtoProcHttp1Factory:public RxProtoProcFactory
{
public:
    RxProtoProcHttp1Factory()=default;
    virtual std::unique_ptr<RxProtoProcessor> new_proto_processor(RxConnection *conn) override;
};

#endif // PROTOCOLHTTP1_H
