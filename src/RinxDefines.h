#ifndef RINXDEFINES_H
#define RINXDEFINES_H

#include "Util/ThreadPool.h"
#include "Util/Singeleton.h"

#define RX_BUFFER_SIZE_LARGE 65536

#define RX_BUFFER_CHUNK_SIZE 4096

#define RX_SIGNO_MAX 128

using g_threadpool=RxSingeleton<RxThreadPool>;


#endif // RINXDEFINES_H
