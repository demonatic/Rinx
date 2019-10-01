#include "ProtocolHttp1.h"
#include "../../Network/Connection.h"
#include "../../Protocol/HTTP/HttpDefines.h"
#include "../../3rd/NanoLog/NanoLog.h"


#define bind_parser_event_to_handler_action(HttpEvent,hdlr_action)\
    _request_parser.register_event(HttpEvent,[this](std::any &data){\
         HttpRequest &req=std::any_cast<HttpRequest&>(data);\
        _req_handler_engine.handle_request(req,[](HttpRequestHandler *hdlr,HttpRequest &req){hdlr->hdlr_action(req);});\
    });


RxProtoHttp1Processor::RxProtoHttp1Processor()
{

}

RxProtoHttp1Processor::~RxProtoHttp1Processor()
{
//    static std::atomic_int count=0;
//    count++;
//    printf("~ProtoHttp1 %d",count.load());
}


void RxProtoHttp1Processor::init(RxConnection &conn)
{
    HttpRequest req;
    req.conn_belongs=&conn;
    conn.data=req;
    bind_parser_event_to_handler_action(HttpEvent::HttpHeaderReceived,on_header_fetched)

    _request_parser.register_event(HttpEvent::ParseError,[](std::any &data){
        HttpRequest &req=std::any_cast<HttpRequest&>(data);
        LOG_INFO<<"parse request error from fd "<<req.conn_belongs->get_rx_fd().raw_fd;
        req.conn_belongs->close();
    });
}

ProcessStatus RxProtoHttp1Processor::process_read_data(RxConnection &conn,RxChainBuffer &input_buf)
{
//    std::cout<<"process http1"<<std::endl;

    _request_parser.parse_from_array(input_buf.readable_begin(),input_buf.readable_end(),conn.data);
    return ProcessStatus::OK;
}

ProcessStatus RxProtoHttp1Processor::handle_write_prepared(RxConnection &conn, RxChainBuffer &output_buf)
{

}

std::unique_ptr<RxProtoProcessor> RxProtoProcHttp1Factory::new_proto_processor(RxConnection &conn)
{
    std::unique_ptr<RxProtoProcessor> http1_processor=std::make_unique<RxProtoHttp1Processor>();
    http1_processor->init(conn);
    return http1_processor;
}
