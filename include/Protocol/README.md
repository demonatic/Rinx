# HTTP处理

由于Rinx中每次input_buffer收到数据即会向上层回调，因此一次并不一定会得到一个完整的请求体，因此解析HTTP请求时需要保存一些中间状态，以便收到后续数据时继续从上次的状态执行。Http Parser设计时参考了做游戏开发时游戏AI逻辑常采用的分层有限状态机-HFSM的方案，在状态比较多时将状态分类，把几个小状态归并到一个高层大状态里，然后再定义高层大状态之间的跳转链接。

![](https://github.com/demonatic/Image-Hosting/blob/master/Rinx/Http_Parser.png)

解析HTTP时定义了如下几个高层状态：StateRequestLine用于解析请求行，StateHeader用于解析Header，StateContentLength用于解析body，StateChunk用于解析chunk编码的body；每个高层状态都是一个有限状态机，维护了一系列小的子状态。每个高层状态都继承自SuperState基类，它主要定义了consume方法来消费HFSM输入的字节流，以及on_entry和on_exit方法，规定了在进出该大状态时需要执行的动作。

```c++
class SuperState{
public:
    SuperState(uint8_t state_id):_id(state_id){ }
	 //定义如何消费字节流来将解析出的内容填充到request中
    virtual void consume(iterable_bytes iterable,void *request)=0;
    virtual void on_entry(const std::any &context={})=0; //进入状态执行的动作(初始化等)
    virtual void on_exit()=0; //退出状态执行的动作(清理一些变量等)

protected:
    uint8_t _id;
    SubState _sub_state; //the current substate
};
```
HFSMParser基类是个变长模板类，模板参数包里面是一系列高层状态SuperState，HFSM构造函数中会将它们一一实例化赋予从0开始自增的id号，并依次放到std::tuple中；HFSM类中有一个_curr_state成员会指向std::tuple中的某一个元素表示当前所在的高层状态，至于高层状态当前处于哪一个子状态则由其自行维护。除此之外HFSM还维护了一个事件表，存储了事件id到事件回调函数的映射，在parse字节流时会根据SuperState需要来调用指定的event callback。
```c++
template<typename... SuperStateTypes>
class HFSMParser{
    //根据类模板参数实例化super_state到tuple中
   HFSMParser():_super_states(make_super_states<SuperStateTypes...>()){
        _curr_state=std::get<0>(_super_states).get(); //默认模板类第一个参数为起始状态
        _curr_state->on_entry();
   }
   //super_state之间的迁移
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

    ///<event,function>
    std::map<int,std::shared_ptr<void>> _event_map;
};
```

HFSM的parse接口使用template接收任意迭代器类型，然后在遍历迭代器，将字节流喂给_curr_state指向的当前高层状态进行消费，消费过程中可能会发生高层状态之间的迁移和event的触发。需要注意的是SuperState的consume方法是虚函数，而HFSM的parse方法接收的迭代器是个模板，C++没法把template成员函数和虚函数直接用在一起：

> Member function templates cannot be declared virtual. This constraint is imposed because the usual implementation of the virtual function call mechanism uses a fixed-size table with one entry per virtual function. However, the number of instantiations of a member function template is not fixed until the entire program has been translated. In contrast, the ordinary members of class templates can be virtual because their number is fixed when a class is instantiated

规避这个问题的方法之一是使用type-erase方式，实现type-erase的iterator来接收任意迭代器交给consume方法遍历，但type-erase迭代器在执行自增"++"，比较"!="，解引用"*"操作时都会涉及到一次virtual方法的调用，性能比较差。因此这里使用std::function嵌套std::function的方式来遍历字节流；即SuperState在consume方法中会调用iterable_bytes方法，传入element_handler来决定消费每个字节时执行何种动作，然后由parse方法遍历迭代器。

```c++
    template<typename T>
    using elm_handler=std::function<void(T,ConsumeCtx&)>;

    template<typename T>
    using iterable=std::function<void(elm_handler<T> const&)>;

    using iterable_bytes=iterable<Byte const>;
```

```c++
    template<typename InputIterator>  
    ProcessStat parse(InputIterator begin,InputIterator end,void *request){ //HFSM
      while(cur!=end){
            _curr_state->consume([&](SuperState::elm_handler<Byte> f){
                for(;cur!=end;++cur){
                    f(*cur,consume_ctx);
                }
            },request);
       }
    }
```

```c++
void StateRequestLine::consume(iterable_bytes iterable, void *request){
    iterable([=](const char c,ConsumeCtx &ctx){
        switch(this->_sub_state) {
            case S_EXPECT_METHOD:{

            }break;
            case ...
        }
    }
}
```

上面可以看到SuperState使用了简单的switch case方式来实现自己的子状态机。实现一个HttpParser只需实现super state接口定义好它的行为，然后将super state作为类模板参数实例化HFSM类即可：

```c++
using HttpParser=HFSMParser<StateRequestLine,StateHeader,StateContentLength,StateChunk>;
```

整个Http Parser会在刚收到请求头，收到完整的请求头，收到一部分body，收到完整的body，解析发生错误时都产生event，如果外层注册了相应的回调函数则会进行调用。

HttpProtoProcessor在上述所有这些事件上都注册了回调，分别进行如下行为：

* 在刚收到请求头时开启receive header定时器
* 在收到完整请求头时取消原来定时器，开启receive body定时器
* 在收到一部分请求体时将指向实际数据的input buffer slice放入HttpRequest对象的ResponseBody中。
* 收到完整请求时查找HttpRouter中的路由，如果有则调用用户注册的路由方法并且安装head和body filter，如果没有则调用默认的方法进行处理。
* 解析错误时发送bad request状态码并关闭连接。

这样设计一方面可以让HttpParser和Buffer解耦，另一方面只要读入了数据都可以直接喂给HttpParser，由它帮你填好传入的HttpRequest对象，并且根据输入流产生什么行为由HttpParser帮你负责调用，降低心智负担。

![](https://github.com/demonatic/Image-Hosting/blob/master/Rinx/Http_Parser_Callbacks.png)

