#include "ProtocolHttp1.h"
#include "../../Network/Connection.h"
#include "../../Protocol/HTTP/HttpDefines.h"
#include "../../3rd/NanoLog/NanoLog.h"

RxProtoHttp1Processor::RxProtoHttp1Processor()
{
    //TODO　处理Http parser关于HttpReqLifetimeStage
    using InputDataRange=HttpParser::InputDataRange<RxChainBuffer::read_iterator>;

    _request_parser.on_event(HttpParse::HeaderReceived,[](InputDataRange origin_data,HttpRequest *http_request){
        std::cout<<"HeaderReceived"<<std::endl;
        http_request->stage()=HttpReqLifetimeStage::HeaderReceived;
        HttpReqHandler req_handler=HttpRequestRouter::get_route_handler(*http_request,http_request->stage());
        std::any_cast<HttpRequestPipeline&>(http_request->get_connection()->data).queue_req_handler(req_handler);

    });

    _request_parser.on_event(HttpParse::OnPartofBody,[](InputDataRange origin_data,HttpRequest *http_request){
         http_request->body()=http_request->get_connection()->get_input_buf().slice(origin_data.first,origin_data.second);
    });

    _request_parser.on_event(HttpParse::RequestReceived,[](InputDataRange origin_data,HttpRequest *http_request){
        std::cout<<"RequestReceived"<<std::endl;
        http_request->stage()=HttpReqLifetimeStage::RequestReceived;
        HttpReqHandler req_handler=HttpRequestRouter::get_route_handler(*http_request,http_request->stage());

        std::any_cast<HttpRequestPipeline&>(http_request->get_connection()->data).queue_req_handler([req_handler](HttpRequest &req,HttpResponse &resp){
            if(req_handler)
                req_handler(req,resp);

            if(resp.empty()){
                std::cout<<"empty resp, use default_static_file_handler"<<std::endl;
                HttpRequestRouter::default_static_file_handler(req,resp);
            }

            if(!resp.has_defer_content_provider()){
                std::cout<<"@on_event RequestCompleted"<<std::endl;
                req.stage()=HttpReqLifetimeStage::RequestCompleted;
            }
        });
    });


    //TODO 不能马上close连接　要等前面的处理完 　close连接和异步task有没有冲突？
    _request_parser.on_event(HttpParse::ParseError,[](HttpRequest *http_request,InputDataRange origin_data){

        std::any_cast<HttpRequestPipeline&>(http_request->get_connection()->data).queue_req_handler([](HttpRequest &req,HttpResponse &resp){
            RxConnection *conn=req.get_connection();
            LOG_INFO<<"parse request error from fd "<<conn->get_rx_fd().raw_fd<<" closing connection..";
            conn->close();
        });

    });
}

RxProtoHttp1Processor::~RxProtoHttp1Processor()
{

}

void RxProtoHttp1Processor::init(RxConnection &conn)
{
    conn.data.emplace<HttpRequestPipeline>(&conn);
}

ProcessStatus RxProtoHttp1Processor::process_read_data(RxConnection &conn,RxChainBuffer &input_buf)
{
//    std::cout<<"process http1 incoming data"<<std::endl;
    ///
    /// \brief try handle one pipeline node if pipeline has no request waiting to be served
    ///
    HttpRequestPipeline &pipeline=std::any_cast<HttpRequestPipeline&>(conn.data);

    long n_left=input_buf.readable_end()-input_buf.readable_begin();
    std::cout<<"@process read data: n_left="<<n_left<<std::endl;
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

ProcessStatus RxProtoHttp1Processor::handle_write_prepared(RxConnection &conn, RxChainBuffer &output_buf)
{
    HttpRequestPipeline &pipeline=std::any_cast<HttpRequestPipeline&>(conn.data);
    pipeline.try_handle_remaining();

    return ProcessStatus::OK;
}

std::unique_ptr<RxProtoProcessor> RxProtoProcHttp1Factory::new_proto_processor(RxConnection &conn)
{
    std::unique_ptr<RxProtoProcessor> http1_processor=std::make_unique<RxProtoHttp1Processor>();
    http1_processor->init(conn);
    return http1_processor;
}

