#ifndef HFSMPARSER_H
#define HFSMPARSER_H

#include <optional>
#include <any>
#include <tuple>
#include <memory>
#include <map>
#include <functional>
#include "Util/Util.h"
#include "Util/FunctionTraits.h"

namespace Rinx {

using Byte=uint8_t;

class SuperState{
public:
    using SubState=uint8_t;

    struct ConsumeCtx{
        template<typename...>
        friend class HFSMParser;

        using SuperStateID=size_t;
        using Context=std::any;

        void add_event(int event){
            if(this->events.has_value()) events.value().push_back(event);
            else this->events={event};
            interrupt_iteartion=true;
        }

        void set_error(){
            err=true;
        }

        /// @brief set number of bytes going to be stepped over in iteration
        void iteration_step_over(size_t step){this->n_step_over=step; interrupt_iteartion=true;}

        void transit_super_state(SuperStateID id, Context ctx={}){
            this->next_super_state.emplace(std::make_pair<SuperStateID,Context>(std::move(id),std::move(ctx)));
            interrupt_iteartion=true;
        }

    public:
        size_t n_left;

    private:
        bool err=false;
        bool interrupt_iteartion=false;
        size_t n_step_over=0;
        std::optional<std::vector<int>> events;
        std::optional<std::pair<SuperStateID,Context>> next_super_state;
    };

public:
    template<typename T>
    using elm_handler=std::function<void(T,ConsumeCtx&)>;

    template<typename T>
    using iterable=std::function<void(elm_handler<T> const&)>;

    using iterable_bytes=iterable<Byte const>;

public:
    SuperState(uint8_t state_id):_id(state_id){ }

    virtual ~SuperState()=default;
    uint8_t get_id() const{return _id;}

    /// @param length: length of the iterable_bytes
    virtual void consume(iterable_bytes iterable,void *request)=0;
    virtual void on_entry([[maybe_unused]] const std::any &context={}){}
    virtual void on_exit(){}

protected:
    uint8_t _id;
    SubState _sub_state; //the current substate

    char padding[6];
};

/**
 *  @class The HFSMParser employs a hierarchy state machine to extract a request each time from a sequence designated by iterator
 */
//TODO jump outof error
template<typename... SuperStateTypes>
class HFSMParser
{
public:
    template<typename IteratorType>
    using SkippedRange=std::pair<IteratorType,IteratorType>;

    struct ProcessStat{
        bool parse_err=false;
        bool got_complete_request=false;
        size_t n_remaining;
    };
    HFSMParser():_super_states(make_super_states<SuperStateTypes...>()){
        _curr_state=std::get<0>(_super_states).get();
        _curr_state->on_entry();
    }

    /// @brief try to extract one request and emit events on the fly
    template<typename InputIterator>
    ProcessStat parse(InputIterator begin,InputIterator end,void *request){
        ProcessStat parse_res;
        InputIterator cur=begin;

        while(cur!=end&&!parse_res.got_complete_request&&!parse_res.parse_err){
            SuperState::ConsumeCtx consume_ctx;
            consume_ctx.n_left=end-cur;
            _curr_state->consume([&](SuperState::elm_handler<Byte> f){
                for(;cur!=end;++cur){
                    f(*cur,consume_ctx);
                    consume_ctx.n_left--;
                    if(consume_ctx.interrupt_iteartion){
                        if(!consume_ctx.n_step_over) ++cur;
                        break;
                    }
                }
            },request);

            if(consume_ctx.events){
                for(int event:consume_ctx.events.value()){
                    if(!consume_ctx.n_step_over){
                        this->emit_event(event,request);
                    }
                    else{
                        assert(end-cur>=consume_ctx.n_step_over);
                        this->emit_event(event,request,SkippedRange<InputIterator>(cur,cur+consume_ctx.n_step_over));
                    }
                }
            }

            if(consume_ctx.next_super_state){
                size_t state_id=(*consume_ctx.next_super_state).first;
                std::any &super_state_ctx=(*consume_ctx.next_super_state).second;
                this->transit_super_state(state_id,super_state_ctx);
                //we assume that a complete request has got if the _curr_state go back to the initial super state
                parse_res.got_complete_request=_curr_state->get_id()==0?true:false;
            }
            cur+=consume_ctx.n_step_over;
            parse_res.parse_err=consume_ctx.err;
        }
        parse_res.n_remaining=end-cur;
        return parse_res;
    }

    template<typename Fun>
    void on_event(const int event,const Fun &fun){
        using stl_func_type=std::function<typename function_traits<Fun>::function_type>;
        _event_map[event]=std::make_shared<stl_func_type>(fun);
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

    /// @return false if there is error occur on event handler
    template<typename ...Args>
    bool emit_event(const int event,Args &&...args){
        auto it=_event_map.find(event);
        if(it==_event_map.end())
            return true;

        typedef std::function<bool(decltype(std::forward<Args>(args))...)> fun_type;
        fun_type &f=*static_cast<fun_type*>(it->second.get());

        return f(std::forward<Args>(args)...);
    }

private:
    std::tuple<std::unique_ptr<SuperStateTypes>...> _super_states;
    SuperState *_curr_state; //always points to an item in _super_states

    ///<event,std::function>
    std::map<int,std::shared_ptr<void>> _event_map;
};

} //namespace Rinx

#endif // HFSMPARSER_H
