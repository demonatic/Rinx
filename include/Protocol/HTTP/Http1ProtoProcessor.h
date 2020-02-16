#ifndef HTTP1PROTOPROCESSOR_H
#define HTTP1PROTOPROCESSOR_H

#include "HttpParser.h"
#include "HttpReqRouter.h"
#include "HttpResponse.h"

namespace Rinx {
using HttpParse::HttpParser;

class RxProtoHttp1Processor:public RxProtoProcessor
{
public:
    RxProtoHttp1Processor(RxConnection *conn,HttpRouter *router);

    virtual bool process_read_data(RxConnection *conn,RxChainBuffer &input_buf) override;
    virtual bool handle_write_prepared(RxConnection *conn,RxChainBuffer &output_buf) override;

    /// @return true if current request's corresponding response has completed
    bool send_respond(RxConnection &conn,RxChainBuffer &output_buf,bool &err);

    void prepare_for_next_req();

private:
    using SkippedRange=HttpParser::SkippedRange<RxChainBuffer::read_iterator>;
    void set_parser_callbacks();

private:

    void set_timeout(uint64_t sec){
        this->cancel_read_timer();
        _read_timer.start_timer(sec*1000,[this](){
            printf("time out %ld",_read_timer.get_duration());
            RxConnection *conn=this->get_connection();
            conn->get_eventloop()->queue_work([=](){conn->close();}); //TODO run in loop
            //TODO send timeout
        });
    }

    void cancel_read_timer(){
        if(_read_timer.is_active()){
            _read_timer.stop();
        }
    }


private:
    HttpParser _request_parser;
    size_t req_count=0;
    const HttpRouter *_router;

    bool _got_a_complete_req;
    HttpReqImpl _req;
    HttpRespImpl _resp;

    RxTimer _read_timer;
};

} //namespace Rinx
#endif // HTTP1PROTOPROCESSOR_H
