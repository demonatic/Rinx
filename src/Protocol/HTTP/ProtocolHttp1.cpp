#include "Protocol/HTTP/ProtocolHttp1.h"
#include "Network/Connection.h"
#include "Protocol/HTTP/HttpDefines.h"
#include "3rd/NanoLog/NanoLog.h"


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

std::unique_ptr<RxProtoProcessor> RxProtocolHttp1Factory::new_proto_processor(RxConnection *conn)
{
    std::unique_ptr<RxProtoProcessor> http1_processor=std::make_unique<RxProtoHttp1Processor>(conn,&_router);
    return http1_processor;
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
