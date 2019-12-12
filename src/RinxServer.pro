TEMPLATE = app
CONFIG += console c++17
CONFIG -= app_bundle
CONFIG -= qt

CONFIG(release,debug|release){
    message("NDEBUG")
    DEFINES+=NDEBUG
}


include(../lib_dependency.pri)

SOURCES += \
        3rd/NanoLog/NanoLog.cpp \
        Network/Buffer.cpp \
        Network/Connection.cpp \
        Network/EventLoop.cpp \
        Network/EventPoller.cpp \
        Network/FD.cpp \
        Network/Timer.cpp \
        Network/TimerHeap.cpp \
        Protocol/HTTP/Http1ProtoProcessor.cpp \
        Protocol/HTTP/HttpParser.cpp \
        Protocol/HTTP/HttpReqRouter.cpp \
        Protocol/HTTP/HttpRequest.cpp \
        Protocol/HTTP/HttpResponse.cpp \
        Protocol/HTTP/ProtocolHttp1.cpp \
        Protocol/ProtocolProcessorFactory.cpp \
        Protocol/ProtocolProcessor.cpp \
        Server/Server.cpp \
        Server/Signal.cpp \
        Server/WorkerThreadLoops.cpp \
        Util/Clock.cpp \
        Util/Mutex.cpp \
        Util/ThreadPool.cpp \
        Util/Util.cpp \
        main.cpp

HEADERS += \
        3rd/NanoLog/NanoLog.h \
        Network/Buffer.h \
        Network/Connection.h \
        Network/EventLoop.h \
        Network/EventPoller.h \
        Network/FD.h \
        Network/Listener.h \
        Network/Timer.h \
        Network/TimerHeap.h \
        Protocol/Echo/ProtocolEcho.h \
        Protocol/HFSMParser.hpp \
        Protocol/HTTP/Http1ProtoProcessor.h \
        Protocol/HTTP/HttpDefines.h \
        Protocol/HTTP/HttpParser.h \
        Protocol/HTTP/HttpReqRouter.h \
        Protocol/HTTP/HttpRequest.h \
        Protocol/HTTP/HttpResponse.h \
        Protocol/HTTP/ProtocolHttp1.h \
        Protocol/ProtocolProcessorFactory.h \
        Protocol/ProtocolProcessor.h \
        RinxDefines.h \
        Server/Server.h \
        Server/Signal.h \
        Server/WorkerThreadLoops.h \
        Util/Clock.h \
        Util/FunctionTraits.h \
        Util/Mutex.h \
        Util/Noncopyable.h \
        Util/ObjectAllocator.hpp \
        Util/Singeleton.h \
        Util/ThreadPool.h \
        Util/Util.h
