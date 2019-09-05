TEMPLATE = app
CONFIG += console c++17
CONFIG -= app_bundle
CONFIG -= qt

DEFINES+=DEBUG

include(../lib_dependency.pri)

SOURCES += \
        3rd/NanoLog/NanoLog.cpp \
        Network/Buffer.cpp \
        Network/Connection.cpp \
        Network/Reactor.cpp \
        Network/ReactorEpoll.cpp \
        Network/Socket.cpp \
        Network/Timer.cpp \
        Network/TimerHeap.cpp \
        Protocol/HFSMParser.cpp \
        Protocol/HTTP/HttpParser.cpp \
        Protocol/HTTP/HttpReqHandlerEngine.cpp \
        Protocol/HTTP/HttpRequest.cpp \
        Protocol/HTTP/HttpRequestHandler.cpp \
        Protocol/HTTP/ProtocolHttp1.cpp \
        Protocol/ProtocolProcessorFactory.cpp \
        Protocol/ProtocolProcessor.cpp \
        RinxDefines.cpp \
        Server/ReactorThreadGroup.cpp \
        Server/Server.cpp \
        Util/Clock.cpp \
        Util/Mutex.cpp \
        Util/ThreadPool.cpp \
        Util/Util.cpp \
        main.cpp

HEADERS += \
        3rd/NanoLog/NanoLog.h \
        Network/Buffer.h \
        Network/Connection.h \
        Network/Event.h \
        Network/Reactor.h \
        Network/ReactorEpoll.h \
        Network/Socket.h \
        Network/Timer.h \
        Network/TimerHeap.h \
        Protocol/HFSMParser.h \
        Protocol/HTTP/HttpDefines.h \
        Protocol/HTTP/HttpParser.h \
        Protocol/HTTP/HttpReqHandlerEngine.h \
        Protocol/HTTP/HttpRequest.h \
        Protocol/HTTP/HttpRequestHandler.h \
        Protocol/HTTP/ProtocolHttp1.h \
        Protocol/ProtocolProcessorFactory.h \
        Protocol/ProtocolProcessor.h \
        RinxDefines.h \
        Server/ReactorThreadGroup.h \
        Server/Server.h \
        Util/Clock.h \
        Util/FunctionTraits.h \
        Util/Mutex.h \
        Util/Noncopyable.h \
        Util/ObjectAllocator.hpp \
        Util/Singeleton.h \
        Util/ThreadPool.h \
        Util/Util.h
