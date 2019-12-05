#ifndef RINXDEFINES_H
#define RINXDEFINES_H

#include <string>

///Server
constexpr size_t SignoMax=128;
constexpr size_t IOWorkerNum=1;
constexpr size_t ThreadPoolWorkerNum=8;
constexpr size_t MaxConnectionNum=65535;

///Network
constexpr size_t BufferChunkSize=4096;
constexpr size_t OutputBufSliceThresh=10;

///HTTP
const std::string WebRootPath="/home/demonatic/Projects/web_test_send";
const std::string DefaultWebPage="nginx.html";
const std::string ServerName="Rinx";
//const std::string DefaultWebPage="short.txt";

constexpr uint64_t ReadHeaderTimeout=5; //sec
constexpr uint64_t ReadBodyTimeout=8; //sec
constexpr uint64_t KeepAliveTimeout=15; //sec

#endif // RINXDEFINES_H
