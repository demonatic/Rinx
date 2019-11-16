#ifndef TST_HTTP_REQUEST_PIPELINE_H
#define TST_HTTP_REQUEST_PIPELINE_H

#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>
#include <gmock/gmock.h>
//#include "../../../src/Protocol/HTTP/HttpParser.h"
//#include "../../../src/Protocol/HTTP/HttpRequest.h"
//#include "../../../src/Protocol/HTTP/HttpDefines.h"

using namespace testing;


TEST(pipeline_test, dataset)
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

}

#endif // TST_HTTP_REQUEST_PIPELINE_H
