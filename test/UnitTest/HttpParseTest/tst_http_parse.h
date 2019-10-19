#ifndef TST_HTTP_PARSE_H
#define TST_HTTP_PARSE_H

#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>
#include <gmock/gmock.h>
#include "../../../src/Protocol/HTTP/HttpParser.h"
#include "../../../src/Protocol/HTTP/HttpRequest.h"
#include "../../../src/Protocol/HTTP/HttpDefines.h"

using namespace testing;

class ParserCallback{
public:
    virtual ~ParserCallback()=default;

    virtual void recv_header_callback(HttpRequest &request){(void)request;}

    virtual void parse_error_callback(HttpRequest &request){(void)request;}

    void parse(std::vector<uint8_t> &buf){

        parser.register_event(HttpReqLifetimeStage::HeaderReceived,[this](void *request,std::vector<uint8_t>::iterator,size_t){
            std::cout<<"@recv header cb"<<std::endl;
            HttpRequest *processed_req=static_cast<HttpRequest*>(request);
            processed_req->debug_print_header();
            this->recv_header_callback(*processed_req);
            processed_req->clear();
        });
        parser.register_event(HttpReqLifetimeStage::RequestReceived,[this](void *request,std::vector<uint8_t>::iterator start,size_t length){
            HttpRequest *req=static_cast<HttpRequest*>(request);
            std::cout<<"!! @recv whole request cb, body size="<<length<<std::endl;
            for(int i=0;i<length;i++){
               std::cout<<*(start+i);
            }
            std::cout<<std::endl;
        });

        parser.register_event(HttpReqLifetimeStage::ParseError,[this](void *request,std::vector<uint8_t>::iterator,size_t){
            std::cout<<"@parse error cb"<<std::endl;
            this->parse_error_callback(*static_cast<HttpRequest*>(request));
        });

        HttpRequest req{nullptr};
        long total=std::distance(buf.begin(),buf.end());
        long n_left=total;
        do{
            HttpParser::ParseRes parse_res=parser.parse(buf.begin()+total-n_left,buf.end(),&req);
            n_left=parse_res.n_remaining;

            if(parse_res.got_complete_request){
                std::cout<<"got a request"<<std::endl;
            }

        }while(n_left);

    }

    HttpParser parser;
};

class MockParserCallBack:public ParserCallback{
public:
    MOCK_METHOD1(recv_header_callback,void(HttpRequest &request));
    MOCK_METHOD1(parse_error_callback,void(HttpRequest &request));
};

TEST(http_parse, dataset)
{
    const char request[]="POST /favicon.ico HTTP/1.1\r\n"
                                "Host: 0.0.0.0=5000\r\n"
                                "User-Agent: Mozilla/5.0 (X11; U; Linux i686; en-US; rv:1.9) Gecko/2008061015 Firefox/3.0\r\n"
                                "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n"
                                "Accept-Language: en-us,en;q=0.5\r\n"
                                "Accept-Encoding: gzip,deflate\r\n"
                                "Accept-Charset: ISO-8859-1,utf-8;q=0.7,*;q=0.7\r\n"
                                "Keep-Alive: 300\r\n"
                                "Connection: keep-alive\r\n"
                                "Content-Length: 3\r\n"
                                "Transfer-Encoding: chunked\r\n"
                                "\r\n"
                                "123"
                        "GET /favicon.ico HTTP/1.1\r\n"
                                     "Host: 0.0.0.0=5000\r\n"
                                     "User-Agent: Mozilla/5.0 (X11; U; Linux i686; en-US; rv:1.9) Gecko/2008061015 Firefox/3.0\r\n"
                                     "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n"
                                     "Accept-Language: en-us,en;q=0.5\r\n"
                                     "Accept-Encoding: gzip,deflate\r\n"
                                     "Accept-Charset: ISO-8859-1,utf-8;q=0.7,*;q=0.7\r\n"
                                     "Keep-Alive: 300\r\n"
                                     "Connection: keep-alive\r\n"
                                     "Transfer-Encoding: chunked\r\n"
                                     "\r\n";

    std::vector<uint8_t> buf(request,request+sizeof(request)-1);
    MockParserCallBack moc_cb;
    moc_cb.parse(buf);
}

#endif // TST_HTTP_PARSE_H
