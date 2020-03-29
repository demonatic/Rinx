cmake_minimum_required(VERSION 3.5)

file(GLOB rinx_sources
    ../include/Rinx/3rd/NanoLog/NanoLog.cpp
    Network/Buffer.cpp
    Network/Connection.cpp
    Network/EventLoop.cpp
    Network/EventPoller.cpp
    Network/FD.cpp
    Network/Timer.cpp
    Network/TimerHeap.cpp
    Protocol/HTTP/Http1ProtoProcessor.cpp
    Protocol/HTTP/HttpParser.cpp
    Protocol/HTTP/HttpReqRouter.cpp
    Protocol/HTTP/HttpRequest.cpp
    Protocol/HTTP/HttpResponse.cpp
    Protocol/HTTP/ProtocolHttp1.cpp
    Protocol/ProtocolProcessorFactory.cpp
    Protocol/ProtocolProcessor.cpp
    Server/Server.cpp
    Server/Signal.cpp
    Server/WorkerThreadLoops.cpp
    Util/Clock.cpp
    Util/Mutex.cpp
    Util/ThreadPool.cpp
    Util/Util.cpp
)

set(ALL_SRCS ${ALL_SRCS} ${rinx_sources} CACHE INTERNAL "")

include_directories(../include)
add_library(Rinx SHARED ${rinx_sources})

set(EXECUTABLE_OUTPUT_PATH &{PROJECT_BINARY_DIR}/bin)

install(TARGETS Rinx
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
