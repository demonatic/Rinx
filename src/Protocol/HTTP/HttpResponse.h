#ifndef HTTPRESPONSEWRITER_H
#define HTTPRESPONSEWRITER_H

#include "Network/Buffer.h"
#include "Network/Connection.h"
#include "HttpDefines.h"

class HttpRequestPipeline;
class HttpResponse{
    friend HttpRequestPipeline;

public:
    using BufAllocator=std::function<uint8_t*(size_t length_expect)>;
    using ProvideDone=std::function<void()>; //TODO　换声明的地方

    struct HttpStatusLine{
        HttpStatusCode status_code;
        HttpVersion version;
    };
    using HttpResponseBody=RxChainBuffer;

    HttpResponse(RxConnection *conn);

    HttpResponse& status_code(HttpStatusCode stat_code){
        HttpStatusLine &stat_line=_status_line.first;
        stat_line.status_code=stat_code;
        _status_line.second=ComponentStatus::SET;
        return *this;
    }

    HttpResponse& headers(std::string name,std::string val){
        if(_status_line.second==ComponentStatus::NOT_SET){
            throw std::runtime_error("HttpResponse: please set status code before add any header");
        }

        _headers.first.add(std::move(name),std::move(val));
        _headers.second=ComponentStatus::SET;
        return *this;
    }

    HttpResponseBody &body(){
        if(_status_line.second==ComponentStatus::NOT_SET||_headers.second==ComponentStatus::NOT_SET){
            throw std::runtime_error("HttpResponse: please set request line and headers before set body");
        }
        return _body;
    }

    bool empty(){
        return _status_line.second==ComponentStatus::NOT_SET||_headers.second==ComponentStatus::NOT_SET;
    }

    bool send_file(const std::string &filename,size_t offset=0);

    class DeferContentProvider{
    public:
        enum class Status{
            ExecAsyncTask,
            Providing,
            Done,
        };

        using ProvideAction=std::function<void(BufAllocator allocator,ProvideDone done)>;

    public:
        DeferContentProvider(RxConnection *conn,ProvideAction provide_action,bool async=false)
            :_status(async?Status::ExecAsyncTask:Status::Providing),_provide_action(provide_action),_conn(conn){}

        /// @return whether use provide_action to provide data to output_buf
        bool try_provide_once();

        void try_provide_remaining(){
            while(try_provide_once());
        }

        Status get_status() const;
        void set_status(Status status) noexcept{ this->_status=status; }

    private:
        Status _status;
        ProvideAction _provide_action;
        RxConnection *_conn;
    };

    void set_content_provider(DeferContentProvider::ProvideAction defer_content_provider){
        _defer_content_provider.emplace(DeferContentProvider(_conn_belongs,defer_content_provider,false));
    }

    void set_async_content_provider(std::function<void()> async_task,DeferContentProvider::ProvideAction defer_content_provider);


    void NotFound_404(){
        std::cout<<"@NotFound_404"<<std::endl;
        this->status_code(HttpStatusCode::NOT_FOUND);
        std::string content="Page Not Found";
        this->headers("Content-Length",std::to_string(content.length()));
        this->body()<<content;
    }

    bool has_defer_content_provider() const{
        return _defer_content_provider.has_value();
    }

    DeferContentProvider& get_defer_content_provider(){
        return _defer_content_provider.value();
    }


private:
    ///@brief flush http head and body to connection's output buffer
    void flush();

    bool check_and_wait_content_provider(){
        if(_defer_content_provider.has_value()){
            DeferContentProvider &content_provider=_defer_content_provider.value();
            content_provider.try_provide_remaining();
            if(content_provider.get_status()!=DeferContentProvider::Status::Done){
               return false;
            }
            _defer_content_provider.reset();
        }
        return true;
    }

private:    
    enum class ComponentStatus{
        NOT_SET,
        SET,
        SENT
    };

    std::pair<HttpStatusLine,ComponentStatus> _status_line;
    std::pair<HttpHeaderFields,ComponentStatus> _headers;

    HttpResponseBody _body;
    std::optional<DeferContentProvider> _defer_content_provider;

    RxConnection *_conn_belongs;
};


#endif // HTTPRESPONSEWRITER_H
