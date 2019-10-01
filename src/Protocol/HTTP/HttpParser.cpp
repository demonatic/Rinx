#include "HttpParser.h"

StateRequestLine::~StateRequestLine()
{

}

ConsumeRes StateRequestLine::consume(iterable_bytes iterable, std::any &request)
{
    ConsumeRes consume_res;
    iterable([&](char const& c)->bool{
        switch(this->_sub_state) {
            case S_EXPECT_METHOD:{
                if(c==' '){
                    _sub_state=S_EXPECT_URI;
                }
                else{
                   _stored_method.push_back(c);
                }
            }break;

            case S_EXPECT_URI:{
                if(c==' '){
                    _sub_state=S_EXPECT_VERSION;
                }
                else{
                    _stored_uri.push_back(c);
                }
            }break;

            case S_EXPECT_VERSION:{
                if(c=='\r'){
                    _sub_state=S_EXPECT_CRLF;
                }
                else{
                    _stored_version.push_back(c);
                }
            }break;

            case S_EXPECT_CRLF:{
                if(likely(c=='\n')){
                    HttpRequest &req=std::any_cast<HttpRequest&>(request);
                    Util::to_upper(_stored_method);
                    req.method=to_http_method(_stored_method);
                    Util::to_upper(_stored_version);
                    req.version=to_http_version(_stored_version);
                    req.uri=std::move(_stored_uri);

                    consume_res.event=HttpEvent::RequestLineReceived;
                    consume_res.next_super_state.emplace(std::pair{GET_ID(StateHeader),nullptr});
                }
                else{
                    consume_res.event=HttpEvent::ParseError;
                }
                return ExitConsume;
            }
        }
        return InConsuming;
    });
    return consume_res;
}



StateHeader::~StateHeader()
{

}

ConsumeRes StateHeader::consume(iterable_bytes iterable, std::any &request)
{
    ConsumeRes consume_res;
    iterable([&](char const& c)->bool{
        switch(this->_sub_state) {
            case S_EXPECT_FIELD_KEY:{
                if(c==':'){
                    _sub_state=S_GET_COLON;
                }
                else{
                    _header_field_key.push_back(c);
                }
            }break;

            case S_GET_COLON:{
                if(c!=' '){
                    _header_field_val.push_back(c);
                }
                _sub_state=S_EXPECT_FIELD_VAL;
            }break;

            case S_EXPECT_FIELD_VAL:{
                if(c=='\r'){
                    _sub_state=S_EXPECT_FIELD_END;
                    HttpRequest &req=std::any_cast<HttpRequest&>(request);

                    req.header_fields.emplace_back(std::make_pair(std::move(_header_field_key),std::move(_header_field_val)));

                }
                else{
                    _header_field_val.push_back(c);
                }
            }break;

            case S_EXPECT_FIELD_END:{
                if(likely(c=='\n')){
                    _sub_state=S_FIELD_ENDED;
                }
                else{
                    consume_res.event=HttpEvent::ParseError;
                    return ExitConsume;
                }
            }break;

            case S_FIELD_ENDED:{
                if(c=='\r'){
                    _sub_state=S_EXPECT_HEADER_END;
                }
                else{
                    _header_field_key.push_back(c);
                    _sub_state=S_EXPECT_FIELD_KEY;
                }
            }break;

            case S_EXPECT_HEADER_END:{
                if(likely(c=='\n')){
                    consume_res.event=HttpEvent::HttpHeaderReceived;
                    consume_res.next_super_state.emplace(std::pair{GET_ID(StateRequestLine),nullptr});
                    return ExitConsume;
                }
                else{
                    consume_res.event=HttpEvent::ParseError;
                    return ExitConsume;
                }
            }
        }
        return InConsuming;
    });
    return consume_res;
}
