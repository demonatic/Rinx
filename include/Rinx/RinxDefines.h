#ifndef RINXDEFINES_H
#define RINXDEFINES_H

#include <string>

namespace Rinx {

///Server
constexpr size_t IOWorkerNum=1;
constexpr size_t ThreadPoolWorkerNum=8;
constexpr size_t MaxConnectionNum=65535;

///Network
constexpr size_t BufferChunkSize=9012;
constexpr size_t OutputBufSliceThresh=10;

///HTTP
constexpr uint64_t ReadHeaderTimeout=5; //sec
constexpr uint64_t ReadBodyTimeout=10; //sec
constexpr uint64_t KeepAliveTimeout=300; //sec

} //namespace Rinx
#endif // RINXDEFINES_H
