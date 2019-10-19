#ifndef HTTPPARSER_H
#define HTTPPARSER_H

#include <string>
#include "HttpRequest.h"
#include "HttpDefines.h"
#include "../HFSMParser.hpp"
#include <iostream>

class StateRequestLine:public SuperState{
public:
    StateRequestLine(uint8_t state_id):SuperState(state_id){
        _stored_method.reserve(10);
        _stored_uri.reserve(100);
        _stored_version.reserve(12);
    }

    virtual ~StateRequestLine() override;

    virtual ConsumeRes consume(size_t length,iterable_bytes iterable,void *request) override;

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
    StateHeader(uint8_t state_id):SuperState(state_id){
        _header_field_key.reserve(100);
        _header_field_val.reserve(100);
    }
    virtual ~StateHeader() override;

    virtual ConsumeRes consume(size_t length,iterable_bytes iterable,void *request) override;

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

    virtual ConsumeRes consume(size_t length,iterable_bytes iterable,void *request) override;

    virtual void on_entry(const std::any &context) override{
        std::cout<<"@content length on entry"<<std::endl;
        this->_length_got=0;
        this->_length_expect=std::any_cast<int>(context);
    }

private:
    size_t _length_got;
    size_t _length_expect;
};



using HttpParser=HFSMParser<StateRequestLine,StateHeader,StateContentLength>;
#define GET_ID(StateType)  HttpParser::get_state_id<StateType>()

#endif // HTTPPARSER_H
