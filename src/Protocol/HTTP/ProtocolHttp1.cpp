#include "ProtocolHttp1.h"
#include "../../Network/Connection.h"
#include "../../Protocol/HTTP/HttpDefines.h"
#include "../../3rd/NanoLog/NanoLog.h"

RxProtoHttp1Processor::RxProtoHttp1Processor()
{

}


void RxProtoHttp1Processor::init(RxConnection &conn)
{
    HttpRequest req;
    req.conn_belongs=&conn;
    conn.data=req;
    bind_parser_event_to_handler_action(HttpEvent::HttpHeaderReceived,on_header_fetched)

    _request_parser.register_event(HttpEvent::ParseError,[](std::any &data){
        HttpRequest req=std::any_cast<HttpRequest>(data);
        LOG_INFO<<"parse request error from fd "<<req.conn_belongs->get_rx_fd().raw_fd;
        req.conn_belongs->close();
    });
}

ProcessStatus RxProtoHttp1Processor::process(RxConnection &conn, RxChainBuffer &buf)
{
//    std::cout<<"process http1"<<std::endl;

    _request_parser.parse_from_array(buf.readable_begin(),buf.readable_end(),conn.data);
    return ProcessStatus::OK;
}

std::unique_ptr<RxProtoProcessor> RxProtoProcHttp1Factory::new_proto_processor(RxConnection &conn)
{
    std::unique_ptr<RxProtoProcessor> http1_processor=std::make_unique<RxProtoHttp1Processor>();
    http1_processor->init(conn);
    return http1_processor;
}
