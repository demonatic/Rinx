#include "ProtocolHttp1.h"
#include "../../Network/Connection.h"

RxProtoHttp1Processor::RxProtoHttp1Processor()
{

}

void RxProtoHttp1Processor::init()
{
//    this->_request_parser=std::make_unique<HttpParser>();
}

ProcessStatus RxProtoHttp1Processor::process(RxConnection &conn, RxChainBuffer &buf)
{
    std::cout<<"process http1"<<std::endl;
    std::vector<uint8_t> v;
//    std::iterator_traits<decltype(buf.readable_begin())>::reference;
      _request_parser.parse_from_array(buf.readable_begin(),buf.readable_end(),conn.data);
//      if(parse_res.has_error()){
//          return ProcessStatus::Proto_Parse_Error;
//      }
//      if(!parse_res.request_received()){
//          return ProcessStatus::Request_Incoming;
//      }
//      HttpRequest request=parse_res.get_request<HttpRequest>();
//      s_req_handler_engine.process_request(request);
}

std::unique_ptr<RxProtoProcessor> RxProtoProcHttp1Factory::new_proto_processor()
{
    std::unique_ptr<RxProtoProcessor> http1_processor=std::make_unique<RxProtoHttp1Processor>();
    http1_processor->init();
    return http1_processor;
}
