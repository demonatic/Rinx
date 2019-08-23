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
    virtual void recv_request_line_callback(HttpRequest &request){(void)request;}

    virtual void recv_header_callback(HttpRequest &request){(void)request;}

    virtual void parse_error_callback(HttpRequest &request){(void)request;}

    void parse(std::vector<uint8_t> &buf){
        parser.register_event(HttpEvent::RequestLineReceived,[this](HttpRequest &request){
            std::cout<<"@recv request line cb"<<std::endl;
            this->recv_request_line_callback(request);
        });
        parser.register_event(HttpEvent::HttpHeaderReceived,[this](HttpRequest &request){
            std::cout<<"@recv header cb"<<std::endl;
            this->recv_header_callback(request);
        });
        parser.register_event(HttpEvent::ParseError,[this](HttpRequest &request){
            std::cout<<"@parse error cb"<<std::endl;
            this->parse_error_callback(request);
        });

        std::any req=HttpRequest{};
        parser.parse_from_array(buf.begin(),buf.end(),req);
        HttpRequest &processed_req=std::any_cast<HttpRequest&>(req);
        processed_req.debug_print();
    }

    HttpParser parser;
};

class MockParserCallBack:public ParserCallback{
public:
    MOCK_METHOD1(recv_request_line_callback,void(HttpRequest &request));
    MOCK_METHOD1(recv_header_callback,void(HttpRequest &request));
};

TEST(http_parse, dataset)
{
    const char request[]="GET /favicon.ico HTTP/1.1\r\n"
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

    std::vector<uint8_t> buf(request,request+sizeof(request));
    MockParserCallBack moc_cb;
    moc_cb.parse(buf);
}

#endif // TST_HTTP_PARSE_H
