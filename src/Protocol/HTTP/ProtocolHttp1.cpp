#include "ProtocolHttp1.h"
#include "../../Network/Connection.h"
#include "../../Protocol/HTTP/HttpDefines.h"

RxProtoHttp1Processor::RxProtoHttp1Processor()
{

}

void RxProtoHttp1Processor::init()
{
    _request_parser.register_event(0,[](){});
}

ProcessStatus RxProtoHttp1Processor::process(RxConnection &conn, RxChainBuffer &buf)
{
    std::cout<<"process http1"<<std::endl;
    std::vector<uint8_t> v;
    _request_parser.parse_from_array(buf.readable_begin(),buf.readable_end(),conn.data);
}

std::unique_ptr<RxProtoProcessor> RxProtoProcHttp1Factory::new_proto_processor()
{
    std::unique_ptr<RxProtoProcessor> http1_processor=std::make_unique<RxProtoHttp1Processor>();
    http1_processor->init();
    return http1_processor;
}
