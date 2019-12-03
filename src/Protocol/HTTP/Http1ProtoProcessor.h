#ifndef HTTP1PROTOPROCESSOR_H
#define HTTP1PROTOPROCESSOR_H

#include "HttpParser.h"
#include "HttpReqRouter.h"
#include "HttpResponse.h"

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

protected:
    using InputDataRange=HttpParser::InputDataRange<RxChainBuffer::read_iterator>;

    /// callback when get http header on socket stream
    void on_header_recv(InputDataRange src,HttpRequest *http_request);

    /// callback when get part of http body on socket stream
    void on_part_of_body(InputDataRange src,HttpRequest *http_request);

    /// callback when a complete request has received
    void on_request_recv(InputDataRange src,HttpRequest *http_request);

    /// callback when an http protocol parse error occurs
    void on_parse_error(InputDataRange src,HttpRequest *http_request);

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

    const HttpRouter *_router;

    bool _got_a_complete_req;
    HttpRequest _req;
    HttpResponse _resp;

    RxTimer _read_timer;
};
#endif // HTTP1PROTOPROCESSOR_H
