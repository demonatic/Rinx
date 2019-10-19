#ifndef HFSMPARSER_H
#define HFSMPARSER_H

#include <optional>
#include <any>
#include <tuple>
#include <memory>
#include <map>
#include <functional>
#include "../Util/Util.h"
#include "../Util/FunctionTraits.h"
#include <iostream>

using byte_t=uint8_t;

template<typename T>
using elm_handler=std::function<void(T,bool&)>; //param: element,exit iteration

template<typename T>
using iterable=std::function<void(elm_handler<T> const&)>;

using iterable_bytes=iterable<byte_t const>;


class ConsumeRes
{
    using SuperStateID=size_t;
    using Context=std::any;

public:
    void set_event(int event){
        this->event.emplace(event);
    }

    void set_n_consumed_explicitly(int n){
        this->n_consumed_explicit.emplace(n);
    }

    void transit_super_state(SuperStateID id, std::optional<Context> ctx=std::nullopt){
        this->next_super_state.emplace(std::make_pair<SuperStateID,Context>(std::move(id),std::move(ctx.value())));
    }

    //if it is set, the HFSMParser will regard it as the number of bytes consumed by the SuperState,
    //instread of the step of cur iterator movement by default
    std::optional<int> n_consumed_explicit;
    std::optional<int> event;
    std::optional<std::pair<SuperStateID,Context>> next_super_state;
};

class SuperState{
public:
    using SubState=uint8_t;

public:
    SuperState(uint8_t state_id):_id(state_id){ }

    virtual ~SuperState()=default;
    uint8_t get_id() const{return _id;}

    /// @param length: length of the iterable_bytes
    virtual ConsumeRes consume(size_t length,iterable_bytes iterable,void *request)=0;
    virtual void on_entry([[maybe_unused]]const std::any &context){}
    virtual void on_exit(){}

protected:
    uint8_t _id;
    SubState _sub_state; //the current substate

    char padding[6];
};


template<typename... SuperStateTypes>
class HFSMParser
{
public:
    struct ParseRes{
        bool got_complete_request=false;
        size_t n_remaining;
    };
    HFSMParser():_super_states(make_super_states<SuperStateTypes...>()){
        _curr_state=std::get<0>(_super_states).get();
        _curr_state->on_entry({});
    }

    /// @brief
    template<typename InputIterator>
    ParseRes parse(InputIterator begin,InputIterator end,void *request){
        std::cout<<"@parse begin: end-begin="<<end-begin<<std::endl;
        ParseRes parse_res;
        InputIterator cur=begin,next=begin;

        while(cur!=end&&!parse_res.got_complete_request){
            next=cur;
            ConsumeRes ret=_curr_state->consume(end-cur,[&](elm_handler<byte_t> f){
                bool stop_iteration=false;
                for(;next!=end;++next){
                    f(*next,stop_iteration);
                    if(unlikely(stop_iteration)){++next; break;}
                }
            },request);

            typename InputIterator::difference_type consume_len=ret.n_consumed_explicit?ret.n_consumed_explicit.value():next-cur;
            std::cout<<"@parse consume_length="<<consume_len<<" next-cur="<<next-cur<<std::endl;
            if(ret.event){
                this->emit_event(*ret.event,request,cur,consume_len);
            }
            if(ret.next_super_state){
                size_t event=(*ret.next_super_state).first;
                std::any &context=(*ret.next_super_state).second;
                this->transit_super_state(event,context);
                //we assume that a complete request has got if the _curr_state go back to the initial super state
                parse_res.got_complete_request=_curr_state->get_id()==0?true:false;
            }
            cur+=consume_len;
        }

        parse_res.n_remaining=std::distance(cur,end);
        return parse_res;
    }

    template<typename Fun>
    void register_event(const int event,const Fun &fun){
        static_assert(std::is_same<typename function_traits<Fun>::return_type,void>::value,
                      "event callback return type must be void");
        using stl_func_type=std::function<typename function_traits<Fun>::function_type>;
        _event_map[event]=std::make_shared<stl_func_type>(fun);
    }

    template<typename ...Args>
    bool emit_event(const int event,Args &&...args){
        auto it=_event_map.find(event);
        if(it==_event_map.end())
            return false;

        typedef std::function<void(decltype(std::forward<Args>(args))...)> fun_type;
        fun_type &f=*static_cast<fun_type*>(it->second.get());

        f(std::forward<Args>(args)...);
        return true;
    }


    template<typename SuperStateType>
    static constexpr auto get_state_id(){
        return Util::GetTupleIndex<std::unique_ptr<SuperStateType>,decltype(_super_states)>::value;
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

    ///<event,std::function>
    std::map<int,std::shared_ptr<void>> _event_map;
};


#endif // HFSMPARSER_H
