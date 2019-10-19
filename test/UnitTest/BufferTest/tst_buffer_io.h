#ifndef TST_BUFFER_TEST_H
#define TST_BUFFER_TEST_H

#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>
#include "../../../src/Network/Buffer.h"

using namespace testing;

TEST(buffer_test, dataset)
{
    std::unique_ptr<ChainBuffer> buf=ChainBuffer::create_chain_buffer();
    RxReadRc read_res;
    ssize_t read_bytes=buf->read_fd(STDIN_FILENO,read_res);
    EXPECT_EQ(read_res, RxReadRc::OK);
    std::cout<<"begin-end distance="<<buf->readable_end()-buf->readable_begin()<<std::endl;
    EXPECT_EQ(read_bytes,buf->readable_end()-buf->readable_begin());
    EXPECT_EQ(read_bytes-read_bytes/2,buf->readable_end()-(buf->readable_begin()+read_bytes/2));

    std::cout<<"buf ref num="<<buf->buf_ref_num()<<std::endl;

    std::cout<<"[print ++]"<<std::endl;
    std::vector<int> read_plus_plus,read_plus_equal,read_reverse_minus_minus,read_reverse_minus_equal;
    for(auto it=buf->readable_begin();it!=buf->readable_end();++it){
        read_plus_plus.push_back(*it);
        std::cout<<*it;
    }

    std::cout<<"[print +=1]"<<std::endl;
    for(auto it=buf->readable_begin();it!=buf->readable_end();it+=1){
        read_plus_equal.push_back(*it);
        std::cout<<*it;
    }

    std::cout<<std::endl<<"[print --]"<<std::endl;
    for(auto it=--buf->readable_end();;--it){
        read_reverse_minus_minus.push_back(*it);
        std::cout<<*it;
        if(it==buf->readable_begin()){
            break;
        }
    }

    std::cout<<std::endl<<"[print -=1]"<<std::endl;
    for(auto it=--buf->readable_end();;){
        read_reverse_minus_equal.push_back(*it);
        std::cout<<*it;
        if(it==buf->readable_begin()){
            break;
        }
        it-=1;
    }

    EXPECT_EQ(read_plus_plus,read_plus_equal);
    std::reverse(read_reverse_minus_minus.begin(),read_reverse_minus_minus.end());
    std::reverse(read_reverse_minus_equal.begin(),read_reverse_minus_equal.end());
    EXPECT_EQ(read_plus_equal,read_reverse_minus_minus);
    EXPECT_EQ(read_reverse_minus_minus,read_reverse_minus_equal);

    RxWriteRc write_res;
    std::cout<<std::endl<<"about to write fd------"<<std::endl;
    ssize_t write_bytes=buf->write_fd(STDOUT_FILENO,write_res);
    std::cout<<"has written fd------"<<std::endl;
    std::cout<<"read "<<read_bytes<<"  write "<<write_bytes<<std::endl;
    EXPECT_EQ(write_res, RxWriteRc::OK);
    EXPECT_EQ(read_bytes,write_bytes);
}

TEST(write_data_test,testset){
    std::unique_ptr<ChainBuffer> buf=ChainBuffer::create_chain_buffer();
    std::string str="std::string";
    const char *p=new char(10);
    int i=65;
    float d=888;
    p="c_str_ptr";
    char array[]="c_array";
    *buf<<"string_literal"<<"|"<<str<<"|"<<array<<"|"<<p<<"|"<<i<<"|"<<d;
    RxWriteRc write_res;
    buf->write_fd(STDOUT_FILENO,write_res);
}

TEST(slice_test,testset){
    ChainBuffer buf;
    std::string content="0123456789abcdefg";
    buf<<content;
    size_t offset=3;
    ChainBuffer sliced=buf.slice(buf.readable_begin()+offset,8);
    std::cout<<"-----sliced content print:-------"<<std::endl;
    for(auto it=sliced.readable_begin();it!=sliced.readable_end();++it){
        EXPECT_EQ(content[it-sliced.readable_begin()+offset],*it);
        std::cout<<*it;
    }
    std::cout<<std::endl;
}

#endif // TST_BUFFER_TEST_H
