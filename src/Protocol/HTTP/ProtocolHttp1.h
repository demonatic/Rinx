#ifndef PROTOCOLHTTP1_H
#define PROTOCOLHTTP1_H

#include "../ProtocolProcessor.h"
#include "../ProtocolProcessorFactory.h"
#include "HttpParser.h"
#include "HttpReqHandlerEngine.h"

class RxProtoHttp1Processor:public RxProtoProcessor
{

#define bind_parser_event_to_handler_action(HttpEvent,hdlr_action)\
    _request_parser.register_event(HttpEvent,[this](std::any &data){\
         HttpRequest req=std::any_cast<HttpRequest>(data);\
        _req_handler_engine.handle_request(req,[](HttpRequestHandler *hdlr,HttpRequest &req){hdlr->hdlr_action(req);});\
    });

public:
    RxProtoHttp1Processor();
    virtual void init(RxConnection &conn) override;
    virtual ProcessStatus process(RxConnection &conn, RxChainBuffer &buf) override;

private:
    HttpParser _request_parser;
    HttpReqHandlerEngine _req_handler_engine;
};


class RxProtoProcHttp1Factory:public RxProtoProcFactory
{
public:
    RxProtoProcHttp1Factory()=default;
    virtual std::unique_ptr<RxProtoProcessor> new_proto_processor(RxConnection &conn) override;
};

#endif // PROTOCOLHTTP1_H
