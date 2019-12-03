#ifndef RINXDEFINES_H
#define RINXDEFINES_H

#include "Util/Singeleton.h"

constexpr size_t RxBufferChunkSize=4096;
constexpr size_t RxOutputBufSliceThresh=10;

constexpr int RX_SIGNO_MAX=128;

constexpr size_t IOWorkerNum=1;
constexpr size_t ThreadPoolWorkerNum=8;

///HTTP
const std::string WebRootPath="/home/demonatic/Projects/web_test_send";
const std::string DefaultWebPage="nginx.html";
const std::string ServerName="Rinx";
//const std::string DefaultWebPage="short.txt";

constexpr uint64_t ReadHeaderTimeout=5; //sec
constexpr uint64_t ReadBodyTimeout=8; //sec
constexpr uint64_t KeepAliveTimeout=1; //sec

#endif // RINXDEFINES_H
