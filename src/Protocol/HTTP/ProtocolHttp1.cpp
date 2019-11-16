#include "ProtocolHttp1.h"
#include "../../Network/Connection.h"
#include "../../Protocol/HTTP/HttpDefines.h"
#include "../../3rd/NanoLog/NanoLog.h"

RxProtoHttp1Processor::RxProtoHttp1Processor(RxConnection *conn):RxProtoProcessor(conn)
{
#define REGISTER_PARSER_EVENT_CB(ParserEvent,callback)\
    _request_parser.on_event(HttpParse::ParserEvent,[this](InputDataRange range,HttpRequest *req){this->callback(range,req);})

    REGISTER_PARSER_EVENT_CB(HeaderReceived,on_header_recv);
    REGISTER_PARSER_EVENT_CB(OnPartofBody,on_part_of_body);
    REGISTER_PARSER_EVENT_CB(RequestReceived,on_request_recv);
    REGISTER_PARSER_EVENT_CB(ParseError,on_parse_error);

#undef REGISTER_PARSER_EVENT_CB

    conn->data.emplace<HttpRequestQueue>(conn);
    _read_timer=std::make_unique<RxTimer>(conn->get_eventloop());
//    this->set_timeout(ReadHeaderTimeout);
}

RxProtoHttp1Processor::~RxProtoHttp1Processor()
{
    cancel_read_timer();
}

ProcessStatus RxProtoHttp1Processor::process_read_data(RxConnection *conn,RxChainBuffer &input_buf)
{
//    std::cout<<"process http1 incoming data"<<std::endl;
    /// \brief try handle one pipeline node if pipeline has no request waiting to be served
    HttpRequestQueue &pipeline=std::any_cast<HttpRequestQueue&>(conn->data);

    long n_left=input_buf.readable_end()-input_buf.readable_begin();
//    std::cout<<"@process read data: n_left="<<n_left<<std::endl;
    do{
        HttpParser::ParseRes parse_res=_request_parser.parse(input_buf.readable_begin(),input_buf.readable_end(),&pipeline.back().request);
        input_buf.commit_consume(n_left-parse_res.n_remaining);
        n_left=parse_res.n_remaining;

        if(parse_res.got_complete_request)
            pipeline.prepare_for_next_request();

    }while(n_left);

    pipeline.try_handle_remaining();

    return ProcessStatus::OK;
}

ProcessStatus RxProtoHttp1Processor::handle_write_prepared(RxConnection *conn, RxChainBuffer &output_buf)
{
    HttpRequestQueue &pipeline=std::any_cast<HttpRequestQueue&>(conn->data);
    pipeline.try_handle_remaining();

    return ProcessStatus::OK;
}

void RxProtoHttp1Processor::on_header_recv(RxProtoHttp1Processor::InputDataRange range, HttpRequest *http_request)
{
//    std::cout<<"HeaderReceived"<<std::endl;
    http_request->set_stage(HttpReqLifetimeStage::HeaderReceived);
//    this->set_timeout(ReadBodyTimeout);

    HttpReqHandler req_handler=HttpRequestRouter::get_route_handler(*http_request,http_request->stage());
    std::any_cast<HttpRequestQueue&>(http_request->get_connection()->data).queue_req_handler(req_handler);
}

void RxProtoHttp1Processor::on_part_of_body(RxProtoHttp1Processor::InputDataRange range, HttpRequest *http_request)
{
    http_request->body()=http_request->get_connection()->get_input_buf().slice(range.first,range.second);
}

void RxProtoHttp1Processor::on_request_recv(RxProtoHttp1Processor::InputDataRange range, HttpRequest *http_request)
{
//    std::cout<<"RequestReceived"<<std::endl;
    http_request->set_stage(HttpReqLifetimeStage::RequestReceived);
    HttpReqHandler req_handler=HttpRequestRouter::get_route_handler(*http_request,http_request->stage());

    std::any_cast<HttpRequestQueue&>(http_request->get_connection()->data).queue_req_handler([req_handler](HttpRequest &req,HttpResponse &resp){
        if(req_handler)
            req_handler(req,resp);

        if(resp.empty()){
//            std::cout<<"empty resp, use default_static_file_handler"<<std::endl;
            HttpRequestRouter::default_static_file_handler(req,resp);
        }

        if(!resp.has_defer_content_provider()){
//            std::cout<<"@on_event RequestCompleted"<<std::endl;
            req.set_stage(HttpReqLifetimeStage::RequestCompleted);
        }
    });
}

void RxProtoHttp1Processor::on_parse_error(RxProtoHttp1Processor::InputDataRange range, HttpRequest *http_request)
{
    //TODO 不能马上close连接　要等前面的处理完 　close连接和异步task有没有冲突？
    std::any_cast<HttpRequestQueue&>(http_request->get_connection()->data).queue_req_handler([](HttpRequest &req,HttpResponse &resp){
        RxConnection *conn=req.get_connection();
        LOG_INFO<<"parse request error from fd "<<conn->get_rx_fd().raw<<" closing connection..";
        conn->close();
    });
}

std::unique_ptr<RxProtoProcessor> RxProtoProcHttp1Factory::new_proto_processor(RxConnection *conn)
{
    std::unique_ptr<RxProtoProcessor> http1_processor=std::make_unique<RxProtoHttp1Processor>(conn);
    return http1_processor;
}

