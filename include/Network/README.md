# EventLoop设计与实现

* ####  EventLoop整体流程：

EventLoop在事件循环中会进行如下处理：

2. 检查管理的定时器，对所有发生超时的定时器调用expire callback。
3. 计算EventPoller超时时间，避免因为无限期阻塞在event poller上而使定时器得不到执行。
4. EventPoller阻塞等待IO事件发生，并调用一系列注册进来的IO callback进行IO事件分派；可被信号中断。
5. 执行其他线程post到本线程EventLoop异步任务队列中的任务。
5.  每轮时执行on_finish()处理注册进来的callback，main eventloop在此处会进行异步信号处理。

```c++
int RxEventLoop::start_event_loop()
{
    _is_running=true;
    while(_is_running){
        check_timers(); //检查超时的定时器
        auto timeout=get_poll_timeout(); //计算event poller超时时间
        
        poll_and_dispatch_io(timeout); //分派IO事件
        run_defers(); //执行异步队列中pending的任务
        on_finish(); //每轮循环开始时处理的任务
    }
    quit();
    return 0;
}
```
* #### IO Polling和事件分发

EventLoop使用了常见的Reactor反应器模式来实现事件分发，平时常见的开源项目如Redis也使用了该模式。

​	![](https://github.com/demonatic/Image-Hosting/blob/master/Rinx/Reactor.jpg)

​	Reactor核心为synchronous Event Demultiplexer同步事件多路分解器，主要依靠操作系统提供的系统调用，如select/epoll等，它允许使用一个线程来检查多个文件描述符fd（Socket等）的就绪状态（IO多路复用），例如是否可读可写，如果至少有一个fd就绪则返回。获得了事件后需要调用相应事件的回调函数进行分发，因此还需要提供一个接口让外部能够注册特定事件的处理逻辑。

​	Rinx使用EventPoller类来封装Linux epoll，使用edge trigger模式。Linux epoll_event域包括如下字段：

```c
struct epoll_event
{
  uint32_t events;	/* Epoll events */
  epoll_data_t data;	/* User data variable */
} 
typedef union epoll_data
{
  void *ptr;
  int fd;
  uint32_t u32;
  uint64_t u64;
} epoll_data_t;
```

EventPoller在epoll_ctl时将epoll_event的64字节data域分成两部分，其中高32位用于存储自定义的文件描述符类型RxFDType，低32位存储了文件描述符的值：

```c++
epoll_event.data.u64=(static_cast<uint64_t>(Fd.type)<<32)|static_cast<uint64_t>(Fd.raw);
```

并且将epoll_wait返回的epoll_event中的events域转化为自定义的RxEventType列表：

```c++
std::vector<RxEventType> RxEventPoller::get_rx_event_types(const epoll_event &ep_event)
{
    std::vector<RxEventType> rx_event_types;
    uint32_t evt=ep_event.events;

    if(evt&(EPOLLHUP|EPOLLERR|EPOLLRDHUP)){
       rx_event_types.emplace_back(Rx_EVENT_ERROR);
    }
    if(evt&EPOLLIN){
        rx_event_types.emplace_back(Rx_EVENT_READ);
    }
    if(evt&EPOLLOUT){
        rx_event_types.emplace_back(Rx_EVENT_WRITE);
    }
    return rx_event_types;
}
```

由此，当epoll_wait返回时我们可以知道每一个元素<FD类型,FD号,<事件类型列表>>三类信息。EventLoop内部设置了一个EventHandler二维数组，一维度为FD类型，另一维度为事件类型，因此可以根据FD类型和事件类型查找应该调用的EventHandler。

```c++
using EventHandler=std::function<bool(const RxEvent &event)>;
EventHandler _handler_table[RxEventType::__Rx_EVENT_TYPE_COUNT][RxFDType::__RxFD_TYPE_COUNT];
```
因此EventLoop类的IO事件分发可以对每一个FD上产生的每种类型event都进行查表，调用注册进来的EventHandler：
```c++
int RxEventLoop::poll_and_dispatch_io(int timeout_millsec)
{
    int nfds=_event_poller.wait(timeout_millsec);

    if(nfds>0){
        int dispatched=0;
        epoll_event *ep_events=_event_poller.get_epoll_events();

        for(int i=0;i<nfds;i++){
            RxEvent rx_event;
            rx_event.Fd.raw=static_cast<int>(ep_events[i].data.u64);
            rx_event.Fd.type=static_cast<RxFDType>(ep_events[i].data.u64>>32);
            rx_event.eventloop=this;

            const std::vector<RxEventType> rx_event_types=_event_poller.get_rx_event_types(ep_events[i]);
            for(RxEventType rx_event_type:rx_event_types){
                rx_event.event_type=rx_event_type;
                EventHandler event_handler=_handler_table[rx_event.event_type][rx_event.Fd.type];
                if(!event_handler)
                    continue;

                if(!event_handler(rx_event))
                    break;

                ++dispatched;
            }
        }
        return dispatched;
    }
	...
}
```

* #### 异步任务

  EventLoop关于异步任务的有两部分，一是为其他线程提供post任务到本线程事件循环中执行的方法，二是在此基础上提供了lunch一个异步任务到线程池执行，线程池完成该任务后将finish callback使用方式一投递回原事件循环的方法。
  
  ##### part 1
  
  EventLoop中有三个成员，defer_functors用于保存未执行的functor，使用mutex来互斥访问。
  
  ```c++
     RxFD _event_fd;   
     RxMutex _defer_mutex;
     std::vector<DeferCallback> _defer_functors;
  ```
  queue_work方法用于其他线程异步添加任务到队列，然后唤醒eventloop使可能阻塞在epoll_wait上的事件循环继续进行下去。EventPoller监听event_fd上的可写事件，唤醒时使用::eventfd_write将event fd的event counter值加一。
  ```c++
  void RxEventLoop::queue_work(RxEventLoop::DeferCallback cb)
  {
      {
          RxMutexLockGuard lock(_defer_mutex);
          _defer_functors.push_back(std::move(cb));
      }
      wake_up_loop();
  }
  ```
  
  EventLoop在分派完IO事件后即取得所有投递进来的DeferCallback依次执行。通过std::swap来在O(1)时间内执行完临界区，减小锁等待时间，尽量让线程在mutex处于spin期间即获得锁。
  
  ```c++
  void RxEventLoop::run_defers()
  {
      std::vector<DeferCallback> defers;
      {
          RxMutexLockGuard lock(_defer_mutex);
          defers.swap(_defer_functors);
      }
  
      for(const DeferCallback &defer_cb:defers){
          defer_cb();
      }
  }
  
  ```
  
  ##### part2
  
  Rinx内部实现了一个简单的线程池，并用单例类包装起来。EventLoop中的async方法接收的参数均为task function，task function args以及task执行完成后的finish callback；async函数内部将task和finish_callback用lambda表达式打包进一个函数投递到线程池中，让线程池先执行task任务，执行完毕后调用EventLoop的前述queue _work方法将finish_callback投递回EventLoop的异步队列执行。使用SFINAE提供了两种async版本，一种是task无返回值的，另一种是将task返回值作为finish_callback接收参数的:
  
  ```c++
      template<typename F1,typename F2,typename ...Args>
      std::enable_if_t<false==std::is_same_v<void,std::invoke_result_t<F1,Args...>>>
      async(F1 &&task,F2 &&finish_callback,Args&& ...task_args){
          g_threadpool::get_instance()->post([this,task,&task_args...,finish_callback]() mutable{
              using task_ret_t=std::invoke_result_t<F1,Args...>;
              task_ret_t res=task(std::forward<Args>(task_args)...);
              this->queue_work([=]() mutable{
                  finish_callback(res);
              });
          });
      }
  
      /// @brief same as above with task of none return value
      template<typename F1,typename F2,typename ...Args>
      std::enable_if_t<true==std::is_same_v<void,std::invoke_result_t<F1,Args...>>>
      async(F1 &&task,F2 &&finish_callback,Args&& ...task_args){
          g_threadpool::get_instance()->post([this,task,&task_args...,finish_callback]() mutable{
              task(std::forward<Args>(task_args)...);
              this->queue_work([=]() mutable{
                  finish_callback();
              });
          });
      }
  ```
  
* #### 定时器

  EventLoop提供了定时器机制来定时执行一次或重复执行任务。很多场景下都会使用到定时器，如HTTP KeepAlive timeout。定时器通常有三种常见的实现，分别为二叉堆（如libevent、boost::asio）、红黑树（如Nginx）和时间轮(如Linux 内核)。它们进行插入、检查过期、删除定时器的时间复杂度分别如下：

  |        |  插入  | 检查过期 |  删除  |
  | :----: | :----: | :------: | :----: |
  | 二叉堆 | log(n) |   O(1)   | log(n) |
  | 红黑树 | log(n) |  log(n)  | log(n) |
  | 时间轮 |  O(1)  |   O(1)   |  O(1)  |

  其中时间轮拥有最好的理论性能，而二叉堆性能中规中矩而且实现简单，因此Rinx使用了二叉堆+hash表的方式来实现定时器。二叉堆采用小顶堆，堆中每个heap_entry存clock_gettime函数+超时时间计算得到的timestamp以及Timer对象的指针；每次check_timers()时只要取堆顶元素判断它的timestamp是否大于等于当前的timestamp，如果是则说明该定时器超时了，调用其Timer对象中的timeout_callback并pop出堆，调整堆后继续检查堆中其他entry。如果timer设置了repeat属性，则重新加入堆中。
  
  ```c++
  class RxTimer
  {
      TimerID _id;
      size_t _heap_index;
  	
      bool _is_active;
      bool _is_repeat;
      uint64_t _duration;
      
      TimerCallback _timeout_cb;
      RxEventLoop *_eventloop_belongs;
  }；
      
  class RxTimerHeap{
     struct heap_entry{
          uint64_t expire_time;
          RxTimer *timer;
      };
  
      TimerID _next_timer_id;
      std::vector<heap_entry> _timer_heap;
      std::unordered_map<TimerID,RxTimer *> _timer_id_map;
  }；
  ```
  
  ```c++
  int RxTimerHeap::check_timer_expiry()
  {
      int expired_num=0;
      for(;;){
          heap_entry *entry=this->get_heap_top();
          if(!entry){
              break;
          }
  
          uint64_t now=Clock::get_now_tick();
          if(entry->expire_time>now){
              break;
          }
  
          RxTimer *timer=entry->timer;
          this->pop_heap_top();
  
          timer->set_active(false);
          timer->expired();
          ++expired_num;
  
          if(timer->is_repeat()){
              uint64_t new_expire_time=Clock::get_now_tick()+timer->get_duration();
              this->add_timer(new_expire_time,timer);
              timer->set_active(true);
          }
      }
      return expired_num;
  }
  ```
  
  之所以要使用hash表，是因为某个定时器可能没到超时时间即被用户取消了，因此要根据需要取消的TimerID查找hash表获得Timer对象，得到对象内部的heap_index，根据heap_index从堆中删除该下标处的元素。需要注意的是删除堆中任意下标的元素时，将末尾元素与被删除元素交换并让数组大小减一后，为了维持小顶堆的性质，需要让换上来的元素和父节点比较，如果没有父节点或者值比父节点小，则需要往下筛堆，否则往上筛堆。

* #### 信号处理

  网络编程中通常需要处理一些信号，例如TCP通信中当通信双方的一方close连接时，若另一方接着发数据，会收到一个RST报文，若此时再发送数据时系统会发出一个SIGPIPE信号给进程，如果进程不处理该信号默认会导致进程结束。对于服务器来说我们不希望因为写操作导致异常退出，我们可以统一设置一个SignalManager来管理所有信号:

  ```c++
  using RxSignalHandler=std::function<void(int)>;
  struct RxSignal
  {
      bool setted=false;
      int signal_no=0;
      RxSignalHandler handler=nullptr;
  };
  
  class RxSignalManager{
  public:
      //注册信号handler
      void RxSignalManager::on_signal(int signo, RxSignalHandler handler){
          _signals[signo].signal_no=signo;
          _signals[signo].handler=handler;
  
          struct sigaction act,old_act;
          act.sa_handler=(handler==nullptr)?SIG_IGN:&async_sig_handler;
          sigemptyset(&act.sa_mask);
          act.sa_flags=0;
          if(sigaction(signo,&act,&old_act)>=0){
              _signals[signo].setted=true;
          }
      }
      //OS调用的sig_handler
      void RxSignalManager::async_sig_handler(int signo){
          RxSignalManager::_signo=signo;
      }
  private:
      static volatile sig_atomic_t _signo;
      inline static constexpr size_t SignoMax=128;
      static RxSignal _signals[SignoMax];
  }
  ```

  SignalManager开辟了一个数组来存储用户注册的信号，其中每个元素都包含signal号和handler，使用POSIX标准定义的sigaction调用将安装的signal号和信号处理程序。由于Linux信号处理程序与主程序并发运行，共享同样的全局变量，因此有可能会与主程序和其他处理程序互相干扰；当主程序因为某些原因如系统调用等进入内核，内核在中断处理完准备返回用户态之前会检查进程是否有信号抵达，如果有用户自定义的sighandler会进入用户态执行该sighandler，它与main函数属于两个独立的控制流，分别使用不同的堆栈空间；sighandler返回时再次进入内核态，此时如果没有新的信号要抵达则回到主程序的控制流中执行下一条指令。其过程如下图所示：
  
  ![](https://github.com/demonatic/Image-Hosting/blob/master/Rinx/Linux_Signal.png)
  
  因此我们的信号处理程序要足够简单其在里面只能调用异步信号安全的函数，我们可以只在处理函数内部把发生的signo号赋值给volatile sig_atomic_t类型的类静态成员\_signo。之所以使用volatile是因为信号处理程序更新\_signo，主程序周期性地读\_signo，编译器优化后可能认为主程序中的\_signo值从来没有变化过，因此使用缓存在寄存器中的副本来满足每次对它的引用，如果这样主程序可能永远无法看到信号处理程序更新后的值；使用volatile强迫编译器每次都从内存中读取它的值。使用sig_atomic_t保证对它的读和写是原子的。
  
  设置该静态变量后在主程序中需要周期性地检查它，如果发现它被设置了则查询signal数组中的RxSignal对象，调用用户实际的handler。由于sub eventloop的工作负荷通常比较重，我们把检查信号的任务交给main eventloop去做，main eventloop会在每轮循环结束时调用on_finish()，它会调用check_and_handle_async_signal去做实际的工作。
  
  ```c++
  //由main eventloop在on_finish中调用
  void RxSignalManager::check_and_handle_async_signal() 
  {
      if(_signo){
          trigger_signal(_signo);
          _signo=0;
      }
  }
  
  void RxSignalManager::trigger_signal(int signo)
  {
      if(_signals[signo].setted){
         RxSignalHandler sig_handler=_signals[signo].handler;
         if(sig_handler){
             sig_handler(signo);
         }
      }
  }
  ```