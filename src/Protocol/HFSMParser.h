#ifndef HFSMPARSER_H
#define HFSMPARSER_H

#include <optional>
#include <any>
#include <tuple>
#include <memory>
#include <unordered_map>
#include <functional>
#include "../Util/Util.h"
#include <iostream>

/**
struct RxParseRes{
    using Request=std::any;

    bool has_error() const{ return _parse_res.first; }
    bool request_received() const{ return _parse_res.second.has_value(); }

    template<typename RequestType>
    RequestType get_request(){
        return std::any_cast<RequestType>(_parse_res.second.value());
    }

private:
    std::pair<bool,std::optional<Request>> _parse_res;
};
**/

template<class T>
using elm_handler=std::function<bool(T)>;
template<class T>
using iterable=std::function<void(elm_handler<T> const&)>;

using byte_t=uint8_t;
using iterable_bytes=iterable<byte_t const&>;

struct ConsumeRes
{
    using SuperStateID=size_t; using Context=std::any;
    std::optional<int> event;
    std::optional<std::pair<SuperStateID,Context>> next_super_state;
};

class SuperState{
    using SubState=uint8_t;
public:
    SuperState(uint8_t state_id):id(state_id){ }

    virtual ~SuperState(){}

    virtual ConsumeRes consume(iterable_bytes iterable,std::any &request)=0;
    virtual void on_entry(const std::any &context)=0;
    virtual void on_exit()=0;

    uint8_t id;
    SubState sub_state; //the current substate

    char padding[6];
};

template<typename... SuperStateTypes>
class HFSMParser
{
public:

    HFSMParser():_super_states(make_super_states<SuperStateTypes...>()){
        _curr_state=std::get<0>(_super_states).get();
        _curr_state->on_entry({});
    }

    template<typename InputIterator>
    void parse_from_array(InputIterator begin,InputIterator end,std::any &request){
        while(begin!=end){
            ConsumeRes ret=_curr_state->consume([&](elm_handler<byte_t> f){
                for(auto &it=begin;it!=end;++it){
                    if(unlikely(!f(*it))){++it; break;}
                }
            },request);
            std::cout<<"consume ret"<<std::endl;
            if(ret.event.has_value()){
                this->emit_event(*ret.event);
            }
            if(ret.next_super_state.has_value()){
                const size_t &event=(*ret.next_super_state).first;
                const std::any &context=(*ret.next_super_state).second;
                this->transit_super_state(event,context);
            }
        }
        std::cout<<"done!!"<<std::endl;
    }

    template<typename Fun>
    void register_event(const char *event,const Fun &fun){

    }

    template<typename ...Args>
    bool emit_event(int,Args &&...args){

    }

private:
    template<typename ...STs>
    auto make_super_states(){
        return make_super_states_impl<STs...>(std::index_sequence_for<STs...>{});
    }

    template<typename ...STs,std::size_t... Is>
    auto make_super_states_impl(std::index_sequence<Is...> const&){
        return std::make_tuple(std::make_unique<STs>(Is)...);
    }

    template<typename SuperStateType>
    constexpr auto get_super_state(){
        return std::get<get_state_id<SuperStateType>>(_super_states);
    }

    template<typename SuperStateType>
    static constexpr auto get_state_id(){
        return Util::GetTupleIndex<std::unique_ptr<SuperStateType>,decltype(_super_states)>::value;
    }

    void transit_super_state(const size_t id,const std::any &context){
        _curr_state->on_exit();
        Util::get_tuple_at(_super_states,id,[this,&context](SuperState *next_state){
            _curr_state=next_state;
            next_state->on_entry(context);
        });
    }

private:
    std::tuple<std::unique_ptr<SuperStateTypes>...> _super_states;
    SuperState *_curr_state; //always points to an item in _super_states

    ///<event,func_ptr>
    std::unordered_map<int,std::shared_ptr<void>> _event_map;
};

#endif // HFSMPARSER_H
