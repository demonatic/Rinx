#ifndef HTTPRESPONSEWRITER_H
#define HTTPRESPONSEWRITER_H

#include "Network/Buffer.h"
#include "Network/Connection.h"
#include "HttpDefines.h"


class HttpResponse{
public:

    using ProvideDone=std::function<void()>; //TODO　换声明的地方
    using HttpStatusLine=std::pair<HttpStatusCode,HttpVersion>;
    using HttpResponseBody=RxChainBuffer;
    enum class ComponentStatus{
        NOT_SET,
        SET,
        SENT
    };

    HttpResponse(RxConnection *conn);

    class DeferContentProvider{
    public:
        enum class Status{
            ExecAsyncTask,
            Providing,
            Done,
        };

        using ProvideAction=std::function<void(RxChainBuffer &output_buf,size_t max_length_expected,ProvideDone done)>;

    public:
        DeferContentProvider(RxConnection *conn,ProvideAction provide_action,bool async=false)
            :_status(async?Status::ExecAsyncTask:Status::Providing),_provide_action(provide_action),_conn(conn)
        {

        }

        /// @return whether use provide_action to provide data to output_buf
        bool try_provide_once();

        void try_provide_remaining(){
            while(try_provide_once());
        }

        Status get_status() const{
            return _status;
        }

        void set_status(Status status) noexcept{
            this->_status=status;
        }


    private:
        Status _status;
        ProvideAction _provide_action;
        RxConnection *_conn;
    };

    void set_content_provider(DeferContentProvider::ProvideAction defer_content_provider){
        _defer_content_provider.emplace(DeferContentProvider(_conn_belongs,defer_content_provider,false));
    }

    void set_async_content_provider(std::function<void()> async_task,DeferContentProvider::ProvideAction defer_content_provider);


    bool has_defer_content_provider() const{
        return _defer_content_provider.has_value();
    }

    DeferContentProvider& get_defer_content_provider(){
        return _defer_content_provider.value();
    }

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

    HttpResponse& status_code(HttpStatusCode status){
        HttpStatusLine &stat_line=_status_line.first;
        stat_line.first=status;
        _status_line.second=ComponentStatus::SET;
        return *this;
    }

    HttpResponse& headers(std::string name,std::string val){
        if(_status_line.second==ComponentStatus::NOT_SET){
            throw std::runtime_error("HttpResponse: set status code before add any header");
        }

        _headers.first.add(std::move(name),std::move(val));
        _headers.second=ComponentStatus::SET;
        return *this;
    }

    HttpResponseBody &body();

    ///@brief flush http head and body to connection's output buffer
    void flush();

private:
    std::pair<HttpStatusLine,ComponentStatus> _status_line;
    std::pair<HttpHeaderFields,ComponentStatus> _headers;

    HttpResponseBody _body;
    std::optional<DeferContentProvider> _defer_content_provider;

    RxConnection *_conn_belongs;
};


#endif // HTTPRESPONSEWRITER_H
