#ifndef PROTOCOLHTTP1_H
#define PROTOCOLHTTP1_H

#include "../ProtocolProcessor.h"
#include "../ProtocolProcessorFactory.h"
#include "Http1ProtoProcessor.h"


/// 1.处理完一个HttpRequest后再处理下一个HttpRequest
/// 2.当HttpParser发现一个HttpRequest到来后先截断剩余input_buffer内的数据，即每次最多提取出一个完整的HttpRequest，
/// 如果一次无法响应完则先不继续读input_buffer内的数据进行处理(EventLoop在socket有read事件时仍然会把数据append到input_buffer)，
/// 当connection.data字段（即HttpRequest）中的标志位表明request complete时才能重置request对象，继续处理下一个连接
///

class RxProtocolHttp1Factory:public RxProtocolFactory
{
public:
    RxProtocolHttp1Factory()=default;
    virtual std::unique_ptr<RxProtoProcessor> new_proto_processor(RxConnection *conn) override;

#define HTTP_ROUTE_FUNC_DECLARE(HttpMethod)\
    void HttpMethod(HttpRouter::Route::RoutableURI uri,Responder req_handler)

    HTTP_ROUTE_FUNC_DECLARE(GET);
    HTTP_ROUTE_FUNC_DECLARE(HEAD);
    HTTP_ROUTE_FUNC_DECLARE(POST);
    HTTP_ROUTE_FUNC_DECLARE(PUT);
    HTTP_ROUTE_FUNC_DECLARE(DELETE);
    HTTP_ROUTE_FUNC_DECLARE(PATCH);
    HTTP_ROUTE_FUNC_DECLARE(OPTIONS);

#undef HTTP_ROUTE_FUNC_DECLARE

    RxProtocolHttp1Factory& head_filter(const HttpRouter::Route::RoutableURI &uri,const HeadFilter filter);
    RxProtocolHttp1Factory& body_filter(const HttpRouter::Route::RoutableURI &uri,const BodyFilter filter);

private:
    HttpRouter _router;
};

#endif // PROTOCOLHTTP1_H
