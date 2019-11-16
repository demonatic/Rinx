#ifndef RINXDEFINES_H
#define RINXDEFINES_H

#include "Util/ThreadPool.h"
#include "Util/Singeleton.h"

constexpr size_t RX_BUFFER_CHUNK_SIZE=4;
constexpr size_t RX_OUTPUT_BUF_SLICE_THRESH=10;

constexpr int RX_SIGNO_MAX=128;

using g_threadpool=RxSingeleton<RxThreadPool>;

///HTTP
const std::string WebRootPath="/home/demonatic/Projects/web_test_send";
const std::string DefaultWebPage="nginx.html";

constexpr uint64_t ReadHeaderTimeout=60; //sec
constexpr uint64_t ReadBodyTimeout=60; //sec
constexpr uint64_t KeepAliveTimeout=60; //sec

#endif // RINXDEFINES_H
