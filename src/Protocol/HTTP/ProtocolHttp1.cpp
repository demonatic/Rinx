#include "ProtocolHttp1.h"
#include "../../Network/Connection.h"
#include "../../Protocol/HTTP/HttpDefines.h"
#include "../../3rd/NanoLog/NanoLog.h"


RxProtoHttp1Processor::RxProtoHttp1Processor()
{

}

RxProtoHttp1Processor::~RxProtoHttp1Processor()
{
}


void RxProtoHttp1Processor::init(RxConnection &conn)
{
    HttpRequest req(&conn);
    conn.data=req;

    _request_parser.register_event(HttpReqLifetimeStage::HeaderReceived,[](std::any &data){
//        std::cout<<"HeaderReceived"<<std::endl;
        if(!HttpRequestRouter::route_request(std::any_cast<HttpRequest&>(data),HttpReqLifetimeStage::HeaderReceived)){

        }
    });

    _request_parser.register_event(HttpReqLifetimeStage::ParseError,[](std::any &data){
        HttpRequest &req=std::any_cast<HttpRequest&>(data);
        LOG_INFO<<"parse request error from fd "<<req.get_connection()->get_rx_fd().raw_fd;
        req.get_connection()->close();
    });
}

ProcessStatus RxProtoHttp1Processor::process_read_data(RxConnection &conn,RxChainBuffer &input_buf)
{
//    std::cout<<"process http1 incoming data"<<std::endl;

    _request_parser.parse(input_buf.readable_begin(),input_buf.readable_end(),conn.data);
    RxWriteRc rc;
    conn.send(rc);
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
