#include "HttpParser.h"

namespace HttpParse{

void StateRequestLine::consume(size_t length,iterable_bytes iterable, void *request)
{
    iterable([=](const char c,ConsumeCtx &ctx)->void{ //use macro
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
                    HttpRequest *http_request=static_cast<HttpRequest*>(request);
                    Util::to_upper(_stored_method);
                    http_request->method=to_http_method(_stored_method);
                    Util::to_upper(_stored_version);
                    http_request->version=to_http_version(_stored_version);
                    http_request->uri=std::move(_stored_uri);
                    ctx.transit_super_state(GET_ID(StateHeader));
                }
                else{
                    ctx.add_event(ParseEvent::ParseError);
                }
            }
        }
    });
}

void StateHeader::consume(size_t length,iterable_bytes iterable, void *request)
{
    HttpRequest *http_request=static_cast<HttpRequest*>(request);
    iterable([=](const char c,ConsumeCtx &ctx)->void{
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
                    http_request->headers.add(std::move(_header_field_key),std::move(_header_field_val));
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
                    goto parse_error;
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
                    ctx.add_event(ParseEvent::HeaderReceived);

                    if(auto content_len=http_request->headers.field_val("content-length")){
                        ctx.transit_super_state(GET_ID(StateContentLength),atoi(content_len.value().c_str())); //TODO try catch
                    }
                    else if(auto header_key=http_request->headers.field_val("transfer-encoding")){
                        ctx.transit_super_state(GET_ID(StateChunk));
                    }
                    else{
                        ctx.transit_super_state(GET_ID(StateRequestLine));
                        ctx.add_event(ParseEvent::RequestReceived);
                    }
                }
                else{
                    goto parse_error;
                }
            }
            break;
parse_error:
            ctx.add_event(ParseEvent::ParseError);
        }

    });
}

void StateContentLength::consume(size_t length,iterable_bytes iterable, void *request)
{
    iterable([=](const char,ConsumeCtx &ctx)->void{
        size_t length_increasement=length+_length_got>=_length_expect?_length_expect-_length_got:length;
        _length_got+=length_increasement;
        ctx.iteration_step_over(length_increasement);
        ctx.add_event(ParseEvent::OnPartofBody);

        if(_length_got==_length_expect){
            ctx.transit_super_state(GET_ID(StateRequestLine));
            ctx.add_event(ParseEvent::RequestReceived);
        }
    });
}

void StateChunk::consume(size_t length, iterable_bytes iterable, void *request)
{
    iterable([=](const char c,ConsumeCtx &ctx)->void{
        switch(this->_sub_state){
            case S_EXPECT_LENGTH:{
                if(c!='\r'){
                    _chunk_len_hex.push_back(c);
                }
                else{
                    _sub_state=S_GET_INITIAL_CR;
                }
            }
            break;

            case S_GET_INITIAL_CR:{
                if(unlikely(c!='\n'||!Util::hex_str_to_size_t(_chunk_len_hex,this->_chunk_len_expect))){
                    ctx.add_event(ParseEvent::ParseError);
                }
                else{
                    if(_chunk_len_expect!=0){
                        _sub_state=S_EXPECT_CHUNK_DATA;
                    }
                    else _sub_state=S_FINISH_WAIT_CR;
                }
            }
            break;

            case S_EXPECT_CHUNK_DATA:{
                size_t length_increasement=_chunk_len_got+length>=_chunk_len_expect?_chunk_len_expect-_chunk_len_got:length;
                _chunk_len_got+=length_increasement;
                ctx.iteration_step_over(length_increasement);
                ctx.add_event(HttpParse::OnPartofBody);

                if(_chunk_len_got==_chunk_len_expect){
                    _sub_state=S_EXPECT_CHUNK_CR;
                }
            }
            break;

            case S_EXPECT_CHUNK_CR:{
                if(likely(c=='\r')){
                     _sub_state=S_EXPECT_CHUNK_LF;
                }
                else{
                    goto parse_error;
                }
            }
            break;

            case S_EXPECT_CHUNK_LF:{
                ctx.transit_super_state(GET_ID(StateChunk));
                if(likely(c=='\n')){
                    //start recv next chunk
                    ctx.transit_super_state(GET_ID(StateChunk));
                }
                else{
                    goto parse_error;
                }
            }
            break;

            case S_FINISH_WAIT_CR:{
                if(likely(c=='\r')){
                    _sub_state=S_FINISH_WAIT_LF;
                }
                else{
                    goto parse_error;
                }
            }
            break;

            case S_FINISH_WAIT_LF:{
                if(likely(c=='\n')){
                    //the whole chunk body has received
                    ctx.add_event(ParseEvent::RequestReceived);
                    ctx.transit_super_state(GET_ID(StateRequestLine));
                }
                else{
                    goto parse_error;
                }
            }
            break;

parse_error:
            ctx.add_event(ParseEvent::ParseError);
        }
    });
}

}
