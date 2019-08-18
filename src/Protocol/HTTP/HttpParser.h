#ifndef HTTPPARSER_H
#define HTTPPARSER_H

#include "../HFSMParser.h"
#include <iostream>

class StateRequestLine:public SuperState{
public:
    StateRequestLine(uint8_t state_id):SuperState(state_id){}

    virtual ConsumeRes consume(iterable_bytes iterable,std::any &request) override;

    virtual void on_entry(const std::any &context) override{}
    virtual void on_exit() override{}

};

class StateHeader:public SuperState{
public:
    StateHeader(uint8_t state_id):SuperState(state_id){}

    virtual ConsumeRes consume(iterable_bytes iterable,std::any &request) override;

    virtual void on_entry(const std::any &context) override{}
    virtual void on_exit() override{}

};

using HttpParser=HFSMParser<StateRequestLine,StateHeader>;


#endif // HTTPPARSER_H
