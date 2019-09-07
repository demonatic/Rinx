#include "ProtocolHttp1.h"
#include "../../Network/Connection.h"
#include "../../Protocol/HTTP/HttpDefines.h"

RxProtoHttp1Processor::RxProtoHttp1Processor()
{

}

void RxProtoHttp1Processor::init()
{
    _request_parser.register_event(HttpEvent::HttpHeaderReceived,[](HttpRequest &request){
//        s_req_handler_engine.process_request(request);
    });
}

ProcessStatus RxProtoHttp1Processor::process(RxConnection &conn, RxChainBuffer &buf)
{
    std::cout<<"process http1"<<std::endl;

    ///test start
    conn.get_reactor().async([](int i){
        std::cout<<"in async task i="<<i<<" thread id="<<std::this_thread::get_id()<<std::endl;
        return std::string("hello world");
    },[&](std::string &async_res){
        std::cout<<"in defer cb thread id="<<std::this_thread::get_id()<<std::endl;

        for(auto it=async_res.begin();it!=async_res.end();it++){
            std::cout<<*it;
        }
        std::swap(conn.get_input_buf(),conn.get_output_buf());
        Rx_Write_Res write_res;
        conn.send(write_res);
        assert(write_res==Rx_Write_Res::OK);
    },1);
    conn.get_reactor().async([](){
        std::cout<<"in async task thread id="<<std::this_thread::get_id()<<std::endl;
    },[&](){
        std::cout<<"in defer cb thread id="<<std::this_thread::get_id()<<std::endl;
        conn.close();
    });
    ///test end
//    _request_parser.parse_from_array(buf.readable_begin(),buf.readable_end(),conn.data);
    return ProcessStatus::OK;
}

std::unique_ptr<RxProtoProcessor> RxProtoProcHttp1Factory::new_proto_processor()
{
    std::unique_ptr<RxProtoProcessor> http1_processor=std::make_unique<RxProtoHttp1Processor>();
    http1_processor->init();
    return http1_processor;
}
