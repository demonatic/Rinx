#ifndef NANOLOG_H
#define NANOLOG_H

#include <cstdint>
#include <memory>
#include <string>
#include <iosfwd>
#include <type_traits>
#include <iostream>

namespace nanolog {
    enum class LogLevel : uint8_t { INFO, WARN, CRIT };

    class NanoLogLine{

    public:
        NanoLogLine(LogLevel level,const char *file,const char *function,uint32_t line);
        ~NanoLogLine();

        NanoLogLine(NanoLogLine &&)=default;
        NanoLogLine& operator=(NanoLogLine &&)=default;

        void stringify(std::ostream &os);

        NanoLogLine& operator <<(char arg);
        NanoLogLine& operator <<(int32_t arg);
        NanoLogLine& operator <<(uint32_t arg);
        NanoLogLine& operator <<(int64_t arg);
        NanoLogLine& operator <<(uint64_t arg);
        NanoLogLine& operator <<(double arg);
        NanoLogLine& operator <<(const std::string& arg);

        template<size_t N>
        NanoLogLine& operator <<(const char(&arg)[N]){
            encode(string_literal_t(arg));
            return *this;
        }

        template<typename Arg>
        //函数返回值NanoLogLine&当且仅当std::is_same < Arg, char const * >成立即传入的Arg是char const* 时有效
        typename std::enable_if < std::is_same < Arg, char const * >::value, NanoLogLine& >::type
        operator <<(Arg const &arg){
            encode(arg);
            return *this;
        }

        template<typename Arg>
        typename std::enable_if<std::is_same<Arg,char *>::value,NanoLogLine&>::type
        operator <<(Arg const &arg){
            encode(arg);
            return *this;
        }

        struct string_literal_t{
            explicit string_literal_t(const char *s):s_(s){}
            const char *s_;
        };

    private:
        char *buffer();

        void resize_buffer_if_needed(size_t additional_bytes);

        template<typename Arg>
        void encode(Arg arg);

        template<typename Arg>
        void encode(Arg arg,uint8_t type_id);

        void encode_c_string(const char *arg,size_t length);
        void encode(char *arg);
        void encode(const char *arg);
        void encode(string_literal_t arg);

        void stringify(std::ostream &os,char *start,const char *const end);

    private:
        size_t bytes_used_;
        size_t buffer_size_;
        std::unique_ptr<char[]> heap_buffer_;
        char stack_buffer_[256-2*sizeof(size_t)-sizeof(decltype(heap_buffer_))-8/* Reserved */];
    };

    struct Nanolog{
        /*
         * Ideally this should have been operator+=
         * Could not get that to compile, so here we are...
         */
        bool operator ==(NanoLogLine &);
    };

    void set_log_level(LogLevel level);

    bool is_logged(LogLevel level);

    /*
     * Non guaranteed logging. Uses a ring buffer to hold log lines.
     * When the ring gets full, the previous log line in the slot will be dropped.
     * Does not block producer even if the ring buffer is full.
     * ring_buffer_size_mb - LogLines are pushed into a mpsc ring buffer whose size
     * is determined by this parameter. Since each LogLine is 256 bytes,
     * ring_buffer_size = ring_buffer_size_mb * 1024 * 1024 / 256
     */

    struct NonGuaranteedLogger{
        NonGuaranteedLogger(uint32_t ring_buffer_size_mb):ring_buffer_size_mb_(ring_buffer_size_mb){}
        uint32_t ring_buffer_size_mb_;
    };

    /*
     * Provides a guarantee log lines will not be dropped.
     */

    struct GuaranteedLogger{

    };

    /*
     * Ensure initialize() is called prior to any log statements.
     * log_directory - where to create the logs. For example - "/tmp/"
     * log_file_name - root of the file name. For example - "nanolog"
     * This will create log files of the form -
     * /tmp/nanolog.1.txt
     * /tmp/nanolog.2.txt
     * etc.
     * log_file_roll_size_mb - mega bytes after which we roll to next log file.
     */

    void initialize(GuaranteedLogger gl,std::string const &log_directory,
                    std::string const &log_file_name,uint32_t log_file_role_size_mb);

    void initialize(NonGuaranteedLogger ngl, const std::string &log_directory,
                    const std::string &log_file_name, uint32_t log_file_role_size_mb);

} //namespace nanolog

#define NANO_LOG(LEVEL) nanolog::Nanolog()==nanolog::NanoLogLine(LEVEL,__FILE__,__func__,__LINE__)

#ifdef ENABLE_LOG
#define LOG_INFO nanolog::is_logged(nanolog::LogLevel::INFO) && NANO_LOG(nanolog::LogLevel::INFO)
#define LOG_WARN nanolog::is_logged(nanolog::LogLevel::WARN) && NANO_LOG(nanolog::LogLevel::WARN)
#define LOG_CRIT nanolog::is_logged(nanolog::LogLevel::CRIT) && NANO_LOG(nanolog::LogLevel::CRIT)
#else
    #define LOG_INFO std::cout
    #define LOG_WARN std::cout
    #define LOG_CRIT std::cout
#endif

#endif // NANOLOG_H
