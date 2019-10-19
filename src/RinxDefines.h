#ifndef RINXDEFINES_H
#define RINXDEFINES_H

#include "Util/ThreadPool.h"
#include "Util/Singeleton.h"

constexpr size_t RX_BUFFER_CHUNK_SIZE=4;
constexpr size_t RX_OUTPUT_BUF_CHUNK_THRESH=10;

constexpr int RX_SIGNO_MAX=128;


using g_threadpool=RxSingeleton<RxThreadPool>;


#endif // RINXDEFINES_H
