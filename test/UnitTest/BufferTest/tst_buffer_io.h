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
    std::cout<<"chunk num="<<buf->chunk_num()<<std::endl;

    std::cout<<"[print ++]"<<std::endl;
    std::vector<int> read_plus_plus,read_plus_equal,read_reverse_minus_minus,read_reverse_minus_equal;
    for(auto it=buf->readable_begin();it!=buf->readable_end();++it){
        read_plus_plus.push_back(*it);
        std::cout<<*it<<" ";
    }

    std::cout<<"[print +=1]"<<std::endl;
    for(auto it=buf->readable_begin();it!=buf->readable_end();){
        read_plus_equal.push_back(*it);
        std::cout<<*it<<" ";
        it+=1;
    }

    std::cout<<std::endl<<"[print --]"<<std::endl;
    for(auto it=--buf->readable_end();;--it){
        read_reverse_minus_minus.push_back(*it);
        std::cout<<*it<<" ";
        if(it==buf->readable_begin()){
            break;
        }
    }

    std::cout<<std::endl<<"[print -=1]"<<std::endl;
    for(auto it=--buf->readable_end();;){
        read_reverse_minus_equal.push_back(*it);
        std::cout<<*it<<" ";
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

#endif // TST_BUFFER_TEST_H
