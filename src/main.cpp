#include "Server/Server.h"
#include "Protocol/HTTP/HttpDefines.h"
#include "Protocol/HTTP/HttpRequest.h"
#include "Protocol/HTTP/HttpResponse.h"


int main(){
    RxSignalManager::disable_current_thread_signal();
    nanolog::initialize(nanolog::GuaranteedLogger(),"/tmp/","rinx_log",20);

    /// 用户必须在某个stage一次性写完header,body可以分多次写
//    HttpRequestRouter::GET("/",HttpReqLifetimeStage::HeaderReceived,[](HttpRequest &req,HttpResponse &resp){
//        std::cout<<"@handler /: HeaderReceived"<<std::endl;
//        req.debug_print_header();

//        resp.status_code(HttpStatusCode::OK)
//            .headers("Content-Length","24")
//            .body()<<"response data\n";

//        resp.set_content_provider([](HttpResponse::BufAllocator allocator,HttpResponse::ProvideDone done){
//             uint8_t *buf=allocator(10);
//             std::string data="large data";
//             std::memcpy(buf,data.c_str(),10);
//             done();
//        });

//    });
//    HttpRequestRouter::GET("/",HttpReqLifetimeStage::RequestCompleted,[](HttpRequest &req,HttpResponse &resp){
//        std::cout<<"@handler /: request compleleted"<<std::endl;
//    });

    RxServer server;
    server.listen("0.0.0.0",7789);
    return 0;
}
