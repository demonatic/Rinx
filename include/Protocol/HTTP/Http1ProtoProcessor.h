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

    /// @brief parse input buffer stream and call corresponding callbacks
    /// @note called when has NOT got a complete request
    void extract_and_handle_request(RxConnection *conn,RxChainBuffer &input_buf,bool &err);

    /// @brief send current request's response as much as possible
    /// @return true if current request's corresponding response has completed
    bool try_output_response(RxConnection &conn,RxChainBuffer &output_buf,bool &err);

    /// @brief resume the requests handle process
    /// steps: send existing outputbuf -> flush response to outputbuf -> handle next request
    /// if any of above operation not complete, it would stop at once
    /// @note called when has got a complete request
    void resume(bool &err);

    void prepare_for_next_req();

    static void default_static_file_handler(HttpRequest &req,HttpResponse &resp);

private:
    using SkippedRange=HttpParser::SkippedRange<RxChainBuffer::read_iterator>;
    void set_parser_callbacks();

private:

    void set_timeout(uint64_t sec){
        this->cancel_read_timer();
        _read_timer.start_timer(sec*1000,[this](){
            RxConnection *conn=this->get_connection();
            conn->get_eventloop()->queue_work([=](){conn->close();}); //TODO run in loop
        });
    }

    void cancel_read_timer(){
        if(_read_timer.is_active()){
            _read_timer.stop();
        }
    }

private:
    bool _got_a_complete_req;
    HttpParser _request_parser;

    const HttpRouter *_router;

    detail::HttpReqImpl _req;
    detail::HttpRespImpl _resp;

    RxTimer _read_timer;
};

} //namespace Rinx
#endif // HTTP1PROTOPROCESSOR_H
