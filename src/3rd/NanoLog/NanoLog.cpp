/*
Distributed under the MIT License (MIT)
    Copyright (c) 2016 Karthik Iyengar
Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in the
Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished
to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS
OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "NanoLog.h"
#include <thread>
#include <chrono>
#include <ctime>
#include <tuple>
#include <fstream>
#include <cstring>
#include <atomic>
#include <queue>

namespace{
    /* Returns microseconds since epoch */
    uint64_t timestamp_now()
    {
      return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    }

    /* I want [2016-10-13 00:01:23.528514] */
    void format_timestamp(std::ostream & os, uint64_t timestamp)
    {
      // The next 3 lines do not work on MSVC!
      // auto duration = std::chrono::microseconds(timestamp);
      // std::chrono::high_resolution_clock::time_point time_point(duration);
      // std::time_t time_t = std::chrono::high_resolution_clock::to_time_t(time_point);
      std::time_t time_t = timestamp / 1000000;
      auto gmtime = std::gmtime(&time_t);
      char buffer[32];
      strftime(buffer, 32, "%Y-%m-%d %T.", gmtime);
      char microseconds[7];
      sprintf(microseconds, "%06llu", timestamp % 1000000);
      os << '[' << buffer << microseconds << ']';
    }

    std::thread::id this_thread_id(){
       static thread_local const std::thread::id id = std::this_thread::get_id();
       return id;
    }

    template<typename T,typename Tuple>
    struct TupleIndex;

    template<typename T,typename... Types>
    struct TupleIndex<T,std::tuple<T,Types...>>{
        static constexpr const::std::size_t value=0;
    };

    //得到类型T在SupportedTypes tuple中的下标
    template<typename T,typename U,typename... Types>
    struct TupleIndex<T,std::tuple<U,Types...>>{
        static constexpr const std::size_t value=1+TupleIndex<T,std::tuple<Types...>>::value;
    };


}

namespace nanolog{
    typedef std::tuple<char,uint32_t,uint64_t,int32_t,int64_t,double,NanoLogLine::string_literal_t,char*> SupportedTypes;

    char const * to_string(LogLevel loglevel){
        switch (loglevel){
            case LogLevel::INFO:
             return "INFO";
            case LogLevel::WARN:
             return "WARN";
            case LogLevel::CRIT:
             return "CRIT";
            default:
             return "XXXX";
        }
    }

    NanoLogLine::NanoLogLine(LogLevel level,const char *file,const char *function,uint32_t line)
        : bytes_used_(0),
          buffer_size_(sizeof(stack_buffer_))
    {
        encode<uint64_t>(timestamp_now());
        encode<std::thread::id>(this_thread_id());
        encode<string_literal_t>(string_literal_t(file));
        encode<string_literal_t>(string_literal_t(function));
        encode<uint32_t>(line);
        encode<LogLevel>(level);
    }

    NanoLogLine::~NanoLogLine()= default;

    void NanoLogLine::stringify(std::ostream &os)
    {
        char *b=!heap_buffer_?stack_buffer_:heap_buffer_.get();
        char const * const end=b+bytes_used_;
        uint64_t timestamp = *reinterpret_cast<uint64_t*>(b); b+=sizeof(uint64_t);
        std::thread::id thread_id = *reinterpret_cast<std::thread::id*>(b);   b+=sizeof(std::thread::id);
        string_literal_t file = *reinterpret_cast < string_literal_t * >(b); b += sizeof(string_literal_t);
        string_literal_t function = *reinterpret_cast < string_literal_t * >(b); b += sizeof(string_literal_t);
        uint32_t line = *reinterpret_cast < uint32_t * >(b); b += sizeof(uint32_t);
        LogLevel loglevel=*reinterpret_cast<LogLevel*>(b);  b+=sizeof(LogLevel);

        format_timestamp(os, timestamp);

        os << '[' << to_string(loglevel) << ']'
               << '[' << thread_id << ']'
               << '[' << file.s_ << ':' << function.s_ << ':' << line << "] ";

        stringify(os,b,end);

        os<<std::endl;

        if(loglevel>=LogLevel::CRIT)
            os.flush();
    }


    NanoLogLine &NanoLogLine::operator<<(char arg)
    {
        encode<char>(arg,TupleIndex<char,SupportedTypes>::value);
        return *this;
    }

    NanoLogLine &NanoLogLine::operator <<(int32_t arg)
    {
        encode<int32_t>(arg,TupleIndex<int32_t,SupportedTypes>::value);
        return *this;
    }

    NanoLogLine &NanoLogLine::operator <<(uint32_t arg)
    {
        encode<uint32_t>(arg,TupleIndex<uint32_t,SupportedTypes>::value);
        return *this;
    }

    NanoLogLine &NanoLogLine::operator <<(int64_t arg)
    {
        encode<int64_t>(arg,TupleIndex<int64_t,SupportedTypes>::value);
        return *this;
    }

    NanoLogLine &NanoLogLine::operator <<(uint64_t arg)
    {
        encode<uint64_t>(arg,TupleIndex<uint64_t,SupportedTypes>::value);
        return *this;
    }

    NanoLogLine &NanoLogLine::operator <<(double arg)
    {
        encode<double>(arg,TupleIndex<double,SupportedTypes>::value);
        return *this;
    }

    NanoLogLine &NanoLogLine::operator <<(const std::string &arg)
    {
        encode_c_string(arg.c_str(),arg.length());
        return *this;
    }

    //返回当前buffer未用的第一个字节
    char *NanoLogLine::buffer()
    {
        return !heap_buffer_?&stack_buffer_[bytes_used_]:&(heap_buffer_.get())[bytes_used_];
    }

    void NanoLogLine::resize_buffer_if_needed(size_t additional_bytes)
    {
        size_t required_size=bytes_used_+additional_bytes;

        if(!heap_buffer_){
            //初始heap_buffer至少分配512bytes内存
            buffer_size_=std::max(static_cast<size_t>(512),required_size);
            heap_buffer_.reset(new char[buffer_size_]);
            memcpy(heap_buffer_.get(),stack_buffer_,bytes_used_);
            return;
        }
        else{
            //heap_buffer将会至少resize为原来2倍大小
            buffer_size_=std::max(static_cast<size_t>(2*buffer_size_),required_size);
            std::unique_ptr<char[]> new_heap_buffer(new char[buffer_size_]);
            memcpy(new_heap_buffer.get(),heap_buffer_.get(),bytes_used_);
            heap_buffer_.swap(new_heap_buffer);
        }
    }

    void NanoLogLine::encode_c_string(const char *arg, size_t length)
    {
        if(length==0)
            return;

        resize_buffer_if_needed(1+(length+1));
        char *buff=buffer();
        auto type_id=TupleIndex<char*,SupportedTypes>::value;
        *reinterpret_cast<uint8_t*>(buff++)=static_cast<uint8_t>(type_id);
        memcpy(buff,arg,length+1);
        bytes_used_+=1+(length+1);
    }

    void NanoLogLine::encode(char *arg)
    {
        if(arg!=nullptr)
            encode_c_string(arg,strlen(arg));
    }

    void NanoLogLine::encode(const char *arg)
    {
        if(arg!=nullptr)
            encode_c_string(arg,strlen(arg));
    }

    void NanoLogLine::encode(NanoLogLine::string_literal_t arg)
    {
        encode<string_literal_t>(arg,TupleIndex<string_literal_t,SupportedTypes>::value);
    }


    template<typename Arg>
    void NanoLogLine::encode(Arg arg)
    {
        *reinterpret_cast<Arg*>(buffer())=arg;
        bytes_used_+=sizeof(arg);
    }

    //将type_id+arg信息一起写入buffer中
    template<typename Arg>
    void NanoLogLine::encode(Arg arg, uint8_t type_id)
    {
        resize_buffer_if_needed(sizeof(Arg)+sizeof(uint8_t));
        encode<uint8_t>(type_id);
        encode<Arg>(arg);
    }

    template<typename Arg>
    char *decode(std::ostream &os,char *b,Arg *dummy){
        Arg arg=*reinterpret_cast<Arg*>(b);
        os<<arg;
        return b+sizeof(arg);
    }

    template<>
    char *decode(std::ostream &os,char *b,NanoLogLine::string_literal_t *dummy){
        NanoLogLine::string_literal_t s=*reinterpret_cast<NanoLogLine::string_literal_t*>(b);
        os<<s.s_;
        return b+sizeof(NanoLogLine::string_literal_t);
    }

    template<>
    char *decode(std::ostream &os, char *b, char **dummy){
        while(*b!='\0'){
            os<<*b;
            ++b;
        }
        return ++b;
    }

    //递归地解析字段类型并将其值写入ostream
    void NanoLogLine::stringify(std::ostream &os, char *start, const char * const end)
    {
        if(start==end)
            return;

        int type_id=static_cast<int>(*start);
        start++;
        switch (type_id) {
        case 0:
            stringify(os,decode(os,start,static_cast<std::tuple_element<0,SupportedTypes>::type*>(nullptr)),end);
            return;
        case 1:
            stringify(os,decode(os,start,static_cast<std::tuple_element<1,SupportedTypes>::type*>(nullptr)),end);
            return;
        case 2:
            stringify(os,decode(os,start,static_cast<std::tuple_element<2,SupportedTypes>::type*>(nullptr)),end);
            return;
        case 3:
            stringify(os,decode(os,start,static_cast<std::tuple_element<3,SupportedTypes>::type*>(nullptr)),end);
            return;
        case 4:
            stringify(os,decode(os,start,static_cast<std::tuple_element<4,SupportedTypes>::type*>(nullptr)),end);
            return;
        case 5:
            stringify(os,decode(os,start,static_cast<std::tuple_element<5,SupportedTypes>::type*>(nullptr)),end);
            return;
        case 6:
            stringify(os,decode(os,start,static_cast<std::tuple_element<6,SupportedTypes>::type*>(nullptr)),end);
            return;
        case 7:
            stringify(os,decode(os,start,static_cast<std::tuple_element<7,SupportedTypes>::type*>(nullptr)),end);
            return;
        }
    }


    struct SpinLock{
        SpinLock(std::atomic_flag &flag):flag_(flag){
            while(flag_.test_and_set(std::memory_order_acquire));
        }

        ~SpinLock(){
            flag_.clear(std::memory_order_release);
        }

        private:
            std::atomic_flag &flag_;
    };

    struct BufferBase{
        virtual ~BufferBase()=default;
        virtual void push(NanoLogLine &&logline)=0;
        virtual bool try_pop(NanoLogLine &logline)=0;
    };

    /* Multi Producer Single Consumer Ring Buffer */
    class RingBuffer:public BufferBase{
        public:
            struct alignas(64) Item{
                Item()
                    :flag_{ATOMIC_FLAG_INIT}
                    ,written_(0)
                    ,logline_(LogLevel::INFO,nullptr,nullptr,0)
                {
                }

                std::atomic_flag flag_;
                char written_;
                char padding_[256-sizeof(std::atomic_flag)-sizeof(char)-sizeof(NanoLogLine)];
                NanoLogLine logline_;
            };

            //初始化一个有size个Item的环形缓冲区
            RingBuffer(size_t const size)
                :size_(size)
                ,ring_(static_cast<Item*>(std::malloc(size*sizeof(Item))))
                ,write_index_(0)
                ,read_index_(0)
            {
                for(size_t i=0;i<size_;i++){
                    new (&ring_[i]) Item();
                }
                static_assert(sizeof(Item)==256,"Unexpected size!=256");
            }

            ~RingBuffer(){
                for(size_t i=0;i<size_;i++){
                    ring_[i].~Item();
                }
                std::free(ring_);
            }

            void push(NanoLogLine &&logline) override{
                unsigned int write_index=write_index_.fetch_add(1,std::memory_order_relaxed)%size_;
                Item &item=ring_[write_index];
                SpinLock spinlock(item.flag_);
                item.logline_=std::move(logline);
                item.written_=1;
            }

            bool try_pop(NanoLogLine &logline) override{
                Item &item=ring_[read_index_%size_];
                SpinLock lock(item.flag_);
                if(item.written_==1){
                    logline=std::move(item.logline_);
                    item.written_=0;
                    ++read_index_;
                    return true;
                }
                return false;
            }

            RingBuffer(RingBuffer const &)=delete;
            RingBuffer& operator =(RingBuffer const &)=delete;

        private:
            size_t const size_;
            Item *ring_;
            std::atomic<unsigned int> write_index_;
            char pad_[64];
            unsigned int read_index_;
    };

    class Buffer{
    public:
        struct Item{
            Item(NanoLogLine &&logline):logline_(std::move(logline)){}
            char padding[256-sizeof(NanoLogLine)];  //保证整个Item大小为256字节
            NanoLogLine logline_;
        };

        static constexpr const size_t size=32768; //8MB helps reduce memory fragmentation

        Buffer():buffer_(static_cast<Item*>(std::malloc(size*sizeof(Item)))){
            for(size_t i=0;i<=size;i++){
                write_state_[i].store(0,std::memory_order_relaxed);
            }
            static_assert(sizeof(Item)==256,"Unexpected size != 256");
        }

        ~Buffer(){
            unsigned int write_count=write_state_[size].load();
            for(size_t i=0;i<=write_count;i++){
                buffer_[i].~Item();
            }
            std::free(buffer_);
        }

        //Returns true if we need to switch to next buffer
        bool push(NanoLogLine &&logline,unsigned int const write_index){
            new (&buffer_[write_index]) Item(std::move(logline));
            write_state_[write_index].store(1,std::memory_order_release);
            return write_state_[size].fetch_add(1,std::memory_order_acquire)+1==size;
        }

        //读出buffer上read_index位置的数据
        bool try_pop(NanoLogLine &logline,unsigned int const read_index){
            if(write_state_[read_index].load(std::memory_order_acquire)){
                Item &item=buffer_[read_index];
                logline=std::move(item.logline_);
                return true;
            }
            return false;
        }

        Buffer(Buffer const &)=delete;
        Buffer& operator =(Buffer const&)=delete;

    private:
        Item *buffer_;
        std::atomic<unsigned int> write_state_[size+1]; //最后一字节存储总写入数
    };

    class QueueBuffer:public BufferBase{
    public:
        QueueBuffer(QueueBuffer const&)=delete;
        QueueBuffer& operator =(QueueBuffer const&)=delete;

        QueueBuffer():current_read_buffer{nullptr}
                                  ,write_index_(0)
                          ,flag_{ATOMIC_FLAG_INIT}
                                   ,read_index_(0)
        {
            setup_next_write_buffer();
        }

        void push(NanoLogLine &&logline) override{
            unsigned int write_index=write_index_.fetch_add(1,std::memory_order_relaxed);
            if(write_index<Buffer::size){
                if(current_write_buffer_.load(std::memory_order_acquire)->push(std::move(logline),write_index))
                {
                    setup_next_write_buffer();
                }
            }
            else{
                while(write_index_.load(std::memory_order_acquire)>=Buffer::size);
                push(std::move(logline));
            }
        }

        bool try_pop(NanoLogLine &logline) override{
            if(current_read_buffer==nullptr)
                current_read_buffer=get_next_read_buffer();

            Buffer *read_buffer=current_read_buffer;

            if(read_buffer==nullptr)
                return false;

            if(bool success=read_buffer->try_pop(logline,read_index_)){
                read_index_++;
                if(read_index_==Buffer::size){
                    read_index_=0;
                    current_read_buffer=nullptr;
                    SpinLock lock(flag_);
                    buffers_.pop();
                }
                return true;
            }
            return false;
        }

    private:
        void setup_next_write_buffer(){
            std::unique_ptr<Buffer> next_write_buffer(new Buffer());
            current_write_buffer_.store(next_write_buffer.get(),std::memory_order_release);
            SpinLock spinlock(flag_);
            buffers_.push(std::move(next_write_buffer));
            write_index_.store(0,std::memory_order_relaxed);
        }


        Buffer* get_next_read_buffer(){
            SpinLock lock(flag_);
            return buffers_.empty()?nullptr:buffers_.front().get();
        }

    private:
        std::queue<std::unique_ptr<Buffer>> buffers_;
        std::atomic<Buffer*> current_write_buffer_;
        Buffer *current_read_buffer;
        std::atomic<unsigned int> write_index_;
        std::atomic_flag flag_;
        unsigned int read_index_;
    };


    class FileWriter{
    public:
        FileWriter(std::string const &log_directory,std::string const &log_file_name,uint32_t log_file_roll_size_mb)
            :log_file_roll_size_bytes_(log_file_roll_size_mb*1024*1024)
            ,path_and_name_(log_directory+log_file_name)
        {
#ifndef DEBUG
            roll_file();
#endif
        }

        void write(NanoLogLine &logline){
#ifdef DEBUG
            logline.stringify(std::cout);
#else
            auto pos=os_->tellp();
            logline.stringify(*os_);
            bytes_written_=os_->tellp()-pos;
            if(bytes_written_>log_file_roll_size_bytes_){
                roll_file();
            }
#endif
        }


    private:
        void roll_file(){
            if(os_){
                os_->flush();
                os_->close();
            }
            bytes_written_=0;
            os_.reset(new std::ofstream());
            std::string log_file_name=path_and_name_;
            //TODO Optimize this part
            log_file_name.append(".");
            log_file_name.append(std::to_string(++file_number_));
            log_file_name.append(".txt");
            os_->open(log_file_name,std::ofstream::out|std::ofstream::trunc);
        }

        uint32_t file_number_=0;
        std::streamoff bytes_written_=0;
        uint32_t const log_file_roll_size_bytes_;
        const std::string path_and_name_;
        std::unique_ptr<std::ofstream> os_;
    };


    class NanoLogger{
    public:
        NanoLogger(NonGuaranteedLogger ngl,std::string const &log_directory,
                   std::string const &log_file_name,uint32_t log_file_roll_size_mb)
                   :state_(State::INIT)
                   ,buffer_base_(new RingBuffer(std::max(1u,ngl.ring_buffer_size_mb_)*1024*4))
                   ,file_writer_(log_directory,log_file_name,std::max(1u,log_file_roll_size_mb))
                   ,thread_(&NanoLogger::pop,this)
        {
            state_.store(State::READY,std::memory_order_release);
        }

        NanoLogger(GuaranteedLogger ngl,std::string const & log_directory,
                   std::string const &log_file_name,uint32_t log_file_roll_size_mb)
                   :state_(State::INIT)
                   ,buffer_base_(new QueueBuffer())
                   ,file_writer_(log_directory,log_file_name,std::max(1u,log_file_roll_size_mb))
                   ,thread_(&NanoLogger::pop,this)
        {
            state_.store(State::READY,std::memory_order_release);
        }

        ~NanoLogger(){
            state_.store(State::SHUTDOWN);
            thread_.join();
        }

        void add(NanoLogLine &&logline){
            buffer_base_->push(std::move(logline));
        }

        void pop(){
            //wait for constructors to complete and pull all stores down there to this thread
            while(state_.load(std::memory_order_acquire)==State::INIT)
                std::this_thread::sleep_for(std::chrono::microseconds(50));

            NanoLogLine logline(LogLevel::INFO,nullptr,nullptr,0);

            while(state_.load()==State::READY){
                if(buffer_base_->try_pop(logline)){
                    file_writer_.write(logline);
                }
                else
                    std::this_thread::sleep_for(std::chrono::microseconds(50));
            }

            //pop and log all remaining entries
            while(buffer_base_->try_pop(logline)){
                file_writer_.write(logline);
            }
        }

    private:
        enum class State{
            INIT,
            READY,
            SHUTDOWN
        };

        std::atomic<State> state_;
        std::unique_ptr<BufferBase> buffer_base_;
        FileWriter file_writer_;
        std::thread thread_;
    };

    std::unique_ptr<NanoLogger> nanologger;
    std::atomic<NanoLogger*> atomic_nanologger;


    bool Nanolog::operator==(NanoLogLine &logline){
        atomic_nanologger.load(std::memory_order_acquire)->add(std::move(logline));
        return true;
    }

    void initialize(GuaranteedLogger gl, const std::string &log_directory,
                    const std::string &log_file_name, uint32_t log_file_role_size_mb)
    {
        nanologger.reset(new NanoLogger(gl,log_directory,log_file_name,log_file_role_size_mb));
        atomic_nanologger.store(nanologger.get(),std::memory_order_seq_cst);
    }

    void initialize(NonGuaranteedLogger ngl, const std::string &log_directory,
                    const std::string &log_file_name, uint32_t log_file_role_size_mb)
    {
        nanologger.reset(new NanoLogger(ngl,log_directory,log_file_name,log_file_role_size_mb));
        atomic_nanologger.store(nanologger.get(),std::memory_order_seq_cst);
    }

    std::atomic<unsigned int> loglevel={0};

    void set_log_level(LogLevel level){
        loglevel.store(static_cast<unsigned int>(level),std::memory_order_release);
    }

    bool is_logged(LogLevel level){
        return static_cast<unsigned int>(level)>=loglevel.load(std::memory_order_relaxed);
    }

}//namespace nanolog


