#include "Server/Server.h"
#include "Protocol/HTTP/ProtocolHttp1.h"

int main(){
    RxSignalManager::disable_current_thread_signal();
    nanolog::initialize(nanolog::GuaranteedLogger(),"/tmp/","rinx_log",20);

    RxProtocolHttp1Factory http1;

    http1.GET("/",[](HttpRequest &req,HttpResponse &resp,ContentGenerator &generator){
        std::cout<<"@handler /: HeaderReceived"<<std::endl;
//        req.debug_print_header();

        resp.status_code(HttpStatusCode::OK)
            .headers("Content-Length","26")  //24
            .body()<<"response data\n";

//        generator.set_content_provider([&](ContentGenerator::BufAllocator allocator,ContentGenerator::ProvideDone done){
//             uint8_t *buf=allocator(10);
//             std::string data="large data";
//             std::memcpy(buf,data.c_str(),10);
//             done();
//        });

        generator.set_async_content_provider([](){
            sleep(0);
            std::cout<<"sleep timeout"<<std::endl;
        },[&](ContentGenerator::BufAllocator allocator,ContentGenerator::ProvideDone done){
             uint8_t *buf=allocator(10);
             std::string data="large data";
             std::memcpy(buf,data.c_str(),10);
             done();
//             req.close_connection();
        });

    });

    http1.header_filter("/",[](HttpRequest &req,HttpResponse &resp,ChainFilter::Next next){
        std::cout<<"@filter head 1 called"<<std::endl;
        resp.headers("Server","Rinx");
        next();
    });

    http1.header_filter("/",[](HttpRequest &req,HttpResponse &resp,ChainFilter::Next next){
        std::cout<<"@filter head 2 called"<<std::endl;
        resp.headers("Host","localhost");
        next();
    });

    http1.body_filter("/",[](HttpRequest &req,HttpResponse &resp,ChainFilter::Next next){
        std::cout<<"@filter body 1 called"<<std::endl;
        resp.body()<<"*";
        next();
    });

    RxServer server;
    uint16_t port=7777;

    bool listen_ok=false;
    do{
       listen_ok=server.listen("0.0.0.0",port++,http1);
    }while(!listen_ok);

    return 0;
}
