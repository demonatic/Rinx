#ifndef TST_BUFFER_TEST_H
#define TST_BUFFER_TEST_H

#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>
#include <fstream>
#include "Network/Buffer.h"
#include <sys/fcntl.h>

using namespace testing;
using namespace Rinx;

TEST(sock_rw_test, testset)
{
    std::unique_ptr<ChainBuffer> buf=ChainBuffer::create_chain_buffer();
    RxReadRc read_res;

    ssize_t read_bytes=buf->read_from_fd(RxFD{RxFDType::FD_REGULAR_FILE,STDIN_FILENO},read_res);
    EXPECT_EQ(read_res, RxReadRc::OK);
    std::cout<<"begin-end distance="<<buf->end()-buf->begin()<<std::endl;
    EXPECT_EQ(read_bytes,buf->end()-buf->begin());
    EXPECT_EQ(read_bytes-read_bytes/2,buf->end()-(buf->begin()+read_bytes/2));

    for(int i=0;i<read_bytes;i++){
        auto it=buf->begin()+i;
        EXPECT_EQ(it-buf->begin(),i);
    }

    std::cout<<"buf ref num="<<buf->buf_slice_num()<<std::endl;

    std::cout<<"[print ++]"<<std::endl;
    std::vector<int> read_plus_plus,read_plus_equal,read_reverse_minus_minus,read_reverse_minus_equal;
    for(auto it=buf->begin();it!=buf->end();++it){
        read_plus_plus.push_back(*it);
        std::cout<<*it;
    }

    std::cout<<"[print +=1]"<<std::endl;
    for(auto it=buf->begin();it!=buf->end();it+=1){
        read_plus_equal.push_back(*it);
        std::cout<<*it;
    }

    std::cout<<std::endl<<"[print --]"<<std::endl;
    for(auto it=--buf->end();;--it){
        read_reverse_minus_minus.push_back(*it);
        std::cout<<*it;
        if(it==buf->begin()){
            break;
        }
    }

    std::cout<<std::endl<<"[print -=1]"<<std::endl;
    for(auto it=--buf->end();;){
        read_reverse_minus_equal.push_back(*it);
        std::cout<<*it;
        if(it==buf->begin()){
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
    ssize_t write_bytes=buf->write_to_fd(RxFD{RxFDType::FD_REGULAR_FILE,STDOUT_FILENO},write_res);
    std::cout<<"has written fd------"<<std::endl;
    std::cout<<"read "<<read_bytes<<"  write "<<write_bytes<<std::endl;
    EXPECT_EQ(write_res, RxWriteRc::OK);
    EXPECT_EQ(read_bytes,write_bytes);
}


TEST(file_rw_test,testset){
    std::ofstream source_file("./test.txt");
    std::string source_str="!!!I love C plus plus\n";
    source_file<<source_str;
    source_file.close();

    std::unique_ptr<ChainBuffer> buf=ChainBuffer::create_chain_buffer();
    std::string prepend="An apple a day keep the doctor away\n";
    std::string append="Now there is no turning back cause mine's dropped off me\n";
    *buf<<prepend;
    EXPECT_EQ(prepend.size(),buf->readable_size());
    RxFD file_read;
    FDHelper::RegFile::open("./test.txt",file_read);
    long file_length=FDHelper::RegFile::get_file_length(file_read);
    std::cout<<"read file length="<<file_length<<std::endl;
    int offset=3;
    bool res=buf->read_from_regular_file(file_read,file_length,offset);
    EXPECT_EQ(prepend.size()+file_length-offset,buf->readable_size());

    *buf<<append;
    EXPECT_EQ(res,true);

    std::string join_str=prepend+source_str.substr(offset)+append;

    size_t i=0;
    EXPECT_EQ(buf->readable_size(),join_str.size());
    for(auto it=buf->begin();it!=buf->end();++it){
        std::cout<<*it;
        EXPECT_EQ(*it,join_str[i++]);
    }

//    RxFD file_write;
//    file_write.raw=::open("./write_file.txt",O_RDWR|O_CREAT,0777);
//    file_write.type=RxFDType::FD_REGULAR_FILE;
//    RxWriteRc rc;
//    buf->write_to_fd(file_write,rc);
//    EXPECT_EQ(rc,RxWriteRc::OK);
//    FDHelper::close(file_write);

//    RxFD file_write_check;
//    EXPECT_EQ(FDHelper::RegFile::open("./write_file.txt",file_write_check),true);
//    buf->read_from_regular_file(file_write_check,FDHelper::RegFile::get_file_length(file_write_check));
//    size_t j=0;
//    for(auto it=buf->begin();it!=buf->end();++it){
//        std::cout<<*it;
//        EXPECT_EQ(*it,join_str[j++]);
//    }
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
    buf->write_to_fd(RxFD{RxFDType::FD_REGULAR_FILE,STDOUT_FILENO},write_res);
}

TEST(slice_test,testset){
    ChainBuffer buf;
    std::string content="0123456789abcdefg";
    buf<<content;
    size_t offset=3;
    ChainBuffer sliced=buf.slice(buf.begin()+offset,buf.begin()+offset+8);
    std::cout<<"-----sliced content print:-------"<<std::endl;
    for(auto it=sliced.begin();it!=sliced.end();++it){
        EXPECT_EQ(content[it-sliced.begin()+offset],*it);
        std::cout<<*it;
    }
    std::cout<<std::endl;
}

#endif // TST_BUFFER_TEST_H
