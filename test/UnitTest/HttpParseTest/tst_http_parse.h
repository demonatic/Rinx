#ifndef TST_HTTP_PARSE_H
#define TST_HTTP_PARSE_H

#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>
#include <gmock/gmock.h>
#include "../../../src/Protocol/HTTP/HttpParser.h"
#include "../../../src/Protocol/HTTP/HttpRequest.h"
#include "../../../src/Protocol/HTTP/HttpDefines.h"

using namespace testing;
using HttpParse::HttpParser;

class ParserCallback{
public:
    using InputDataRange=std::pair<std::vector<uint8_t>::iterator,std::vector<uint8_t>::iterator>;

    virtual ~ParserCallback()=default;

    virtual void recv_header_callback(HttpRequest &request){(void)request;}

    virtual void parse_error_callback(HttpRequest &request){(void)request;}

    void parse(std::vector<uint8_t> &buf){

        parser.on_event(HttpParse::HeaderReceived,[this](InputDataRange origin_data,HttpRequest *http_request){
            std::cout<<"@recv header cb"<<std::endl;
            http_request->debug_print_header();
            this->recv_header_callback(*http_request);
            http_request->clear();
        });
        parser.on_event(HttpParse::RequestReceived,[this](InputDataRange origin_data,HttpRequest *http_request){
            std::cout<<"@recv whole request cb"<<std::endl;
        });

        parser.on_event(HttpParse::ParseError,[this](InputDataRange origin_data,HttpRequest *http_request){
            std::cout<<"@parse error cb"<<std::endl;
            this->parse_error_callback(*http_request);
        });

        parser.on_event(HttpParse::OnPartofBody,[](InputDataRange origin_data,HttpRequest *http_request){
             size_t body_length=origin_data.second-origin_data.first;
             std::cout<<"body length ="<<body_length<<std::endl;
             for(auto it=origin_data.first;it!=origin_data.second;++it){
                 std::cout<<*it;
             }
             std::cout<<std::endl;
        });

        HttpRequest req{nullptr};
        long total=std::distance(buf.begin(),buf.end());
        long n_left=total;
        do{
            HttpParse::HttpParser::ParseRes parse_res=parser.parse(buf.begin()+total-n_left,buf.end(),&req);
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
                                "Content-Length: 24\r\n"
                                "Transfer-Encoding: chunked\r\n"
                                "\r\n"
                                "test content length data"
                        "GET /favicon.ico HTTP/1.1\r\n"
                                     "Host: 0.0.0.0=5000\r\n"
                                     "User-Agent: Mozilla/5.0 (X11; U; Linux i686; en-US; rv:1.9) Gecko/2008061015 Firefox/3.0\r\n"
                                     "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n"
                                     "Accept-Language: en-us,en;q=0.5\r\n"
                                     "Accept-Encoding: gzip,deflate\r\n"
                                     "Accept-Charset: ISO-8859-1,utf-8;q=0.7,*;q=0.7\r\n"
                                     "Keep-Alive: 300\r\n"
                                     "Connection: keep-alive\r\n"
                                     "\r\n"
                         "POST /post_identity_body_world?q=search#hey HTTP/1.0\r\n"
                         "Accept: */*\r\n"
                         "Transfer-Encoding: chunked\r\n"
                         "\r\n"
                         "19\r\n"
                         "CiXin Liu-Wandering Earth\r\n"
                         "f\r\n"
                         "Part-Time Lover\r\n"
                         "0\r\n"
                         "\r\n"
                             "GET /fiction.txt HTTP/1.0\r\n"
                             "Host: 8.8.8.8=5000\r\n"
                             "Accept-Encoding: gzip,deflate\r\n"
                             "Content-Length: 30\r\n"
                             "\r\n"
                             "A mind at peace with all below"
                     ;

    std::vector<uint8_t> buf(request,request+sizeof(request)-1);
    MockParserCallBack moc_cb;
    moc_cb.parse(buf);
}

#endif // TST_HTTP_PARSE_H
