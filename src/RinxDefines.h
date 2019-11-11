#ifndef RINXDEFINES_H
#define RINXDEFINES_H

#include "Util/ThreadPool.h"
#include "Util/Singeleton.h"

constexpr size_t RX_BUFFER_CHUNK_SIZE=4;
constexpr size_t RX_OUTPUT_BUF_SLICE_THRESH=10;

constexpr int RX_SIGNO_MAX=128;

const std::string WebRootPath="/home/demonatic/Projects/web_test_send";
const std::string DefaultWebPage="nginx.html";

using g_threadpool=RxSingeleton<RxThreadPool>;


#endif // RINXDEFINES_H
