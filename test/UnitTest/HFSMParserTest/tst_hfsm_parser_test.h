#ifndef TST_HFSM_PARSER_TEST_H
#define TST_HFSM_PARSER_TEST_H

#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>
#include "../../../src/Protocol/HFSMParser.hpp"

using namespace testing;

enum events{
    event_0,
    event_1
};

struct TRequest{
    static constexpr int val=1;
    void cb_1(TRequest &request){
        std::cout<<"@f cb_1 "<<TRequest::val<<std::endl;
    }

    void cb_2(TRequest &request){
        std::cout<<"@f cb_2 "<<TRequest::val<<std::endl;
    }
};

inline std::vector<uint8_t> read_in;

class State0:public SuperState{
public:
    State0(uint8_t state_id):SuperState(state_id){}

    virtual ConsumeRes consume(size_t length,iterable_bytes iterable,void *request) override{
        ConsumeRes ret;
        iterable([&](const byte_t byte,bool &)->bool{
            read_in.push_back(byte);
            ret.event=event_0;
            ret.next_super_state.emplace(std::pair{1,666});
        });
        return ret;
    }

    virtual void on_entry(const std::any &context) override{}
    virtual void on_exit() override{}
};

class State1:public SuperState{
public:
    State1(uint8_t state_id):SuperState(state_id){}

    virtual ConsumeRes consume(size_t length,iterable_bytes iterable,void *request) override{
        ConsumeRes ret;
        iterable([&](const byte_t byte,bool &)->bool{
            read_in.push_back(byte);
            ret.event=event_1;
            ret.next_super_state.emplace(std::pair{0,"hello world"});
        });
        return ret;
    }

    virtual void on_entry(const std::any &context) override{}
    virtual void on_exit() override{}
};

using TestParser=HFSMParser<State0,State1>;

TEST(hfsm_parser_test, dataset)
{
    std::function<void()> func=nullptr;
    if(!func){
        std::cout<<"!!!"<<std::endl;
    }

    TestParser parser;
    TRequest request;
    parser.register_event(event_0,[&](void *req){request.cb_1(*static_cast<TRequest*>(req));});
    parser.register_event(event_1,[&](void *req){request.cb_2(*static_cast<TRequest*>(req));});
    std::vector<uint8_t> buf{0,1,2,3,4,5,6};

    parser.parse(buf.begin(),buf.end(),&request);
    EXPECT_EQ(read_in,buf);

    EXPECT_EQ(1, 1);
    ASSERT_THAT(0, Eq(0));
}

#endif // TST_HFSM_PARSER_TEST_H
