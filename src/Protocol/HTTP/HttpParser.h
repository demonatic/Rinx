#ifndef HTTPPARSER_H
#define HTTPPARSER_H

#include <string>
#include "HttpRequest.h"
#include "HttpDefines.h"
#include "../HFSMParser.hpp"
#include <iostream>

namespace HttpParse{
enum ParseEvent{
    ParseError, //TODO
    HeaderReceived,
    OnPartofBody,
    RequestReceived, //the whole reqeust has received
};

class StateRequestLine:public SuperState{
public:
    StateRequestLine(uint8_t state_id):SuperState(state_id){}

    virtual void consume(size_t length,iterable_bytes iterable,void *request) override;

    virtual void on_entry(const std::any &context) override{
        _sub_state=S_EXPECT_METHOD;
    }
    virtual void on_exit() override{
        _stored_method.clear();
        _stored_uri.clear();
        _stored_version.clear();
    }

private:
    std::string _stored_method;
    std::string _stored_uri;
    std::string _stored_version;

    enum SubStates{
        S_EXPECT_METHOD,
        S_EXPECT_URI,
        S_EXPECT_VERSION,
        S_EXPECT_CRLF
    };
};

class StateHeader:public SuperState{
public:
    StateHeader(uint8_t state_id):SuperState(state_id){}

    virtual void consume(size_t length,iterable_bytes iterable,void *request) override;

    virtual void on_entry(const std::any &context) override{
        _sub_state=S_EXPECT_FIELD_KEY;
    }
    virtual void on_exit() override{
        _header_field_key.clear();
        _header_field_val.clear();
    }

private:
    enum SubStates {
        S_EXPECT_FIELD_KEY,
        S_GET_COLON,
        S_EXPECT_FIELD_VAL,
        S_EXPECT_FIELD_END,
        S_FIELD_ENDED,
        S_EXPECT_HEADER_END,
    };

    std::string _header_field_key;
    std::string _header_field_val;
};

class StateContentLength:public SuperState{
public:
    StateContentLength(uint8_t state_id):SuperState(state_id){}

    virtual void consume(size_t length,iterable_bytes iterable,void *request) override;

    virtual void on_entry(const std::any &context) override{
        this->_length_got=0;
        this->_length_expect=std::any_cast<int>(context);
    }

private:
    size_t _length_got;
    size_t _length_expect;
};


class StateChunk:public SuperState{
public:
    StateChunk(uint8_t state_id):SuperState(state_id){}
    virtual void consume(size_t length,iterable_bytes iterable,void *request) override;
    
    virtual void on_entry(const std::any &context) override{
        _chunk_len_expect=0;
        _chunk_len_got=0;
        _chunk_len_hex.clear();
        _sub_state=S_EXPECT_LENGTH;
    }
private:
    std::string _chunk_len_hex;
    size_t _chunk_len_expect;
    size_t _chunk_len_got;

    enum SubStates {
        S_EXPECT_LENGTH,
        S_GET_INITIAL_CR,
        S_EXPECT_CHUNK_DATA,
        S_EXPECT_CHUNK_CR,
        S_EXPECT_CHUNK_LF,
        //finish the chunk recv
        S_FINISH_WAIT_CR,
        S_FINISH_WAIT_LF,
    };
};

using HttpParser=HFSMParser<StateRequestLine,StateHeader,StateContentLength,StateChunk>;
#define GET_ID(StateType)  HttpParser::get_state_id<StateType>()

} //namespace HttpParser
#endif // HTTPPARSER_H
