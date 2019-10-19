#include "HttpParser.h"

StateRequestLine::~StateRequestLine()
{

}

ConsumeRes StateRequestLine::consume(size_t length,iterable_bytes iterable, void *request)
{
    ConsumeRes consume_res;
    iterable([&](const char c,bool &stop_iteration)->void{ //use macro
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
                    HttpRequest *req=static_cast<HttpRequest*>(request);
                    Util::to_upper(_stored_method);
                    req->method()=to_http_method(_stored_method);
                    Util::to_upper(_stored_version);
                    req->version()=to_http_version(_stored_version);
                    req->uri()=std::move(_stored_uri);

                    consume_res.next_super_state.emplace(std::pair{GET_ID(StateHeader),nullptr});
                }
                else{
//                    consume_res.transit_super_state(HttpReqLifetimeStage::ParseError);
                    consume_res.event.emplace(HttpReqLifetimeStage::ParseError);
                }

                stop_iteration=true; //use macro
                return;
            }
        }
    });
    return consume_res;
}



StateHeader::~StateHeader()
{

}

ConsumeRes StateHeader::consume(size_t length,iterable_bytes iterable, void *request)
{
    ConsumeRes consume_res;
    HttpRequest *req=static_cast<HttpRequest*>(request);

    iterable([&](const char c,bool &stop_iteration)->void{
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
                    Util::to_lower(_header_field_key);
                    Util::to_lower(_header_field_val);
                    req->headers().add(std::move(_header_field_key),std::move(_header_field_val));
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
                    consume_res.event=HttpReqLifetimeStage::ParseError;
                    stop_iteration=true;
                    return;
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
                    consume_res.event.emplace(HttpReqLifetimeStage::HeaderReceived);
                    HttpRequest *req=static_cast<HttpRequest*>(request);
                    if(auto content_len=req->headers().field_val("content-length")){
                        consume_res.next_super_state.emplace(std::pair{GET_ID(StateContentLength),atoi(content_len.value().c_str())});
                    }
                    else if(auto header_key=req->headers().field_val("transfer-encoding")){

                    }
                    else{
                        consume_res.next_super_state.emplace(std::pair{GET_ID(StateRequestLine),nullptr});
                    }
                    stop_iteration=true;
                    return;
                }
                else{
                    consume_res.event=HttpReqLifetimeStage::ParseError;
                    stop_iteration=true;
                    return;
                }
            }
        }
    });
    return consume_res;
}

ConsumeRes StateContentLength::consume(size_t length,iterable_bytes iterable, void *request)
{
    ConsumeRes consume_res;

    size_t length_increasement=length+_length_got>=_length_expect?_length_expect-_length_got:length;
    _length_got+=length_increasement;
    std::cout<<"@length increase "<<length_increasement<<std::endl;
    consume_res.n_consumed_explicit.emplace(length_increasement);

    if(_length_got==_length_expect){
        consume_res.event.emplace(HttpReqLifetimeStage::RequestReceived);
        consume_res.next_super_state.emplace(std::pair{GET_ID(StateRequestLine),nullptr});
    }
    return consume_res;
}
