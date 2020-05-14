#include "Rinx/Protocol/HTTP/ProtocolHttp1.h"
#include "Rinx/Network/Connection.h"
#include "Rinx/Protocol/HTTP/HttpDefines.h"
#include "3rd/NanoLog/NanoLog.hpp"

namespace Rinx {

#define HTTP_ROUTE_FUNC_DEFINE(Method) \
void RxProtocolHttp1Factory::Method(HttpRouter::Route::RoutableURI uri,Responder req_handler){\
    HttpRouter::Route route{HttpMethod::Method,uri};\
    _router.set_responder_route(route,req_handler);\
}
HTTP_ROUTE_FUNC_DEFINE(GET)
HTTP_ROUTE_FUNC_DEFINE(HEAD)
HTTP_ROUTE_FUNC_DEFINE(POST)
HTTP_ROUTE_FUNC_DEFINE(PUT)
HTTP_ROUTE_FUNC_DEFINE(DELETE)
HTTP_ROUTE_FUNC_DEFINE(PATCH)
HTTP_ROUTE_FUNC_DEFINE(OPTIONS)

#undef HTTP_ROUTE_FUNC_DEFINE

RxProtocolHttp1Factory::RxProtocolHttp1Factory()
{
    this->_router.set_default_responder([](HttpRequest &req,HttpResponse &resp){
        resp.send_status(HttpStatusCode::FORBIDDEN);
        req.close_connection();
    });
}

std::unique_ptr<RxProtoProcessor> RxProtocolHttp1Factory::new_proto_processor(RxConnection *conn)
{
    std::unique_ptr<RxProtoProcessor> http1_processor=std::make_unique<RxProtoHttp1Processor>(conn,&_router);
    return http1_processor;
}

void RxProtocolHttp1Factory::default_handler(const Responder responder)
{
    this->_router.set_default_responder(responder);
}

RxProtocolHttp1Factory &RxProtocolHttp1Factory::head_filter(const HttpRouter::Route::RoutableURI &uri, const HeadFilter filter)
{
    _router.set_head_filter_route(uri,filter);
    return *this;
}

RxProtocolHttp1Factory &RxProtocolHttp1Factory::body_filter(const HttpRouter::Route::RoutableURI &uri, const BodyFilter filter)
{
    _router.set_body_filter_route(uri,filter);
    return *this;
}

void RxProtocolHttp1Factory::set_www(const std::string &web_root_dir,const std::string &default_web_page)
{
    this->_router.set_www(web_root_dir,default_web_page);
    this->_router.set_default_responder(std::bind(
        &RxProtoHttp1Processor::default_static_file_handler,std::placeholders::_1,
            std::placeholders::_2,_router.get_www_dir(),_router.get_default_web_page()));
}

} //namespace Rinx
