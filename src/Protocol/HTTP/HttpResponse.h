#ifndef HTTPRESPONSEWRITER_H
#define HTTPRESPONSEWRITER_H

#include "../../Network/Buffer.h"
#include "../../Network/Connection.h"
#include "HttpDefines.h"
#include "HttpRequest.h"
#include <queue>

class RxProtoHttp1Processor;
class HttpResponse;
/// callable objects to generate response
using Responder=std::function<void(HttpRequest &req,HttpResponse &resp)>;

/// callable objects to filter response
using Next=std::function<void()>;
using BodyFilter=std::function<void(HttpRequest &req,HttpResponseBody &body,Next next)>;
using HeadFilter=std::function<void(HttpRequest &req,HttpStatusLine &status_line,HttpHeaderFields &headers,Next next)>;

struct ContentGenerator:public std::enable_shared_from_this<ContentGenerator>{
    ContentGenerator(RxConnection *conn):_status(Status::Done),_conn_belongs(conn){

    }
    using BufAllocator=std::function<uint8_t*(size_t length_expect)>;
    using ProvideDone=std::function<void()>;
    using AsyncTask=std::function<void()>;

    enum class Status{
        ExecAsyncTask,
        Providing,
        Done,
    };
    using ProvideAction=std::function<void(BufAllocator allocator,ProvideDone done)>;

    void try_provide_once(HttpResponseBody &body);

    void set_async_content_provider(AsyncTask async_task,ProvideAction provide_action);
    void set_content_provider(ProvideAction provide_action);

    Status get_status() const{ return _status; }
    bool done(){return get_status()==Status::Done;}
    void set_status(Status status) noexcept{ this->_status=status; }

    Status _status;
    ProvideAction _provide_action;
    RxConnection *_conn_belongs;
};

class HttpResponse{
    friend class HttpRouter; friend class RxProtoHttp1Processor;

    template<typename Filter>
    struct ChainFilter{
        ChainFilter()=default;
        ChainFilter(HttpRequest &req,const std::list<Filter> &filters):_req(&req),_filters(&filters){}

        /// @brief execute the filters in sequence and return true if all filters are executed successfully

        template<typename ...FilterArgs>
        bool operator()(FilterArgs&& ...args) const{
            if(!_filters)
                return true;

            auto it=_filters->cbegin();
            for(;it!=_filters->cend();++it){
                bool next=false;
                const Filter &filter=*it;
                filter(*_req,std::forward<FilterArgs>(args)...,[&](){
                    next=true;
                });
                if(!next) break;
            }
            return it==_filters->cend();
        }

        operator bool(){
            return _req&&_filters;
        }

    private:
        HttpRequest *_req;
        const std::list<Filter> *_filters;
    };

public:
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

    void content_provider(ContentGenerator::ProvideAction provide_action){
        if(!_generator){
            _generator=std::make_shared<ContentGenerator>(_conn_belongs);
        }
        _generator->set_content_provider(provide_action);
    }

    void async_content_provider(ContentGenerator::AsyncTask async_task,ContentGenerator::ProvideAction provide_action){
        if(!_generator){
            _generator=std::make_shared<ContentGenerator>(_conn_belongs);
        }
        _generator->set_async_content_provider(async_task,provide_action);
    }

    /// @brief put http header and the file content as http body into outputbuf
    bool send_file(const std::string &filename,size_t offset=0);
    /// @brief send file immediately to socket without going through filters and outputbuf
    bool send_file_direct(RxConnection *conn,const std::string &filename,size_t offset=0);

    void send_status(HttpStatusCode code){
        std::string content=to_http_status_code_str(code);
        this->status_code(code).headers("Content-Length",std::to_string(content.size())).body()<<content;
    }

    void clear();
    bool empty() const;

private:
    /// @brief flush http head and body to connection's output buffer while calling filters
    /// @return false if error occurs
    bool flush(RxChainBuffer &output_buf);

    bool has_block_operation(){
        return _generator&&!_generator->done();
    }

    void install_head_filters(const ChainFilter<HeadFilter> &filters){
        this->_header_filters=filters;
    }

    void install_body_filters(const ChainFilter<BodyFilter> &filters){
        this->_body_filters=filters;
    }

private:
    enum class ComponentStatus{ NOT_SET,SET,SENT };

    std::pair<HttpStatusLine,ComponentStatus> _status_line;
    std::pair<HttpHeaderFields,ComponentStatus> _headers;
    HttpResponseBody _body;

private:
    std::shared_ptr<ContentGenerator> _generator;
    ChainFilter<HeadFilter> _header_filters;
    ChainFilter<BodyFilter> _body_filters;

    RxConnection *_conn_belongs; //TODO owner
};


#endif // HTTPRESPONSEWRITER_H
