#ifndef HTTPPARSER_H
#define HTTPPARSER_H

#include <string>
#include "../HFSMParser.h"
#include <iostream>

enum HttpEvent{
    RequestLineReceived,
    HttpHeaderReceived,
    ParseError,
};

class StateRequestLine:public SuperState{
public:
    StateRequestLine(uint8_t state_id):SuperState(state_id){}

    virtual ConsumeRes consume(iterable_bytes iterable,std::any &request) override;

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

    virtual ConsumeRes consume(iterable_bytes iterable,std::any &request) override;

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

using HttpParser=HFSMParser<StateRequestLine,StateHeader>;
#define GET_ID(StateType)  HttpParser::get_state_id<StateType>()

#endif // HTTPPARSER_H
