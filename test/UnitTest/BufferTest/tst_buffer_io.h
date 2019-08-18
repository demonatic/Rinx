#ifndef TST_BUFFER_TEST_H
#define TST_BUFFER_TEST_H

#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>
#include "../../../src/Network/Buffer.h"

using namespace testing;

TEST(buffer_test, dataset)
{
    std::unique_ptr<ChainBuffer> buf=ChainBuffer::create_chain_buffer();
    Rx_Read_Res read_res;
    ssize_t read_bytes=buf->read_fd(STDIN_FILENO,read_res);
    EXPECT_EQ(read_res, Rx_Read_Res::OK);
    std::cout<<"chunk num="<<buf->chunk_num()<<std::endl;

    std::cout<<"print"<<std::endl;
    for(auto it=buf->readable_begin();it!=buf->readable_end();++it){
        std::cout<<*it<<" ";
    }
    std::cout<<std::endl<<"reverse print:"<<std::endl;
    for(auto it=--buf->readable_end();;--it){
        std::cout<<*it<<" ";
        if(it==buf->readable_begin()){
            break;
        }
    }


    Rx_Write_Res write_res;
    std::cout<<std::endl<<"about to write fd------"<<std::endl;
    ssize_t write_bytes=buf->write_fd(STDOUT_FILENO,write_res);
    std::cout<<"has written fd------"<<std::endl;
    std::cout<<"read "<<read_bytes<<"  write "<<write_bytes<<std::endl;
    EXPECT_EQ(write_res, Rx_Write_Res::OK);
    EXPECT_EQ(read_bytes,write_bytes);
}

#endif // TST_BUFFER_TEST_H
