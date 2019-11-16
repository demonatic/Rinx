include(gtest_dependency.pri)

include(../../../lib_dependency.pri)

TEMPLATE = app
CONFIG += console c++17
CONFIG -= app_bundle
CONFIG += thread
CONFIG -= qt

HEADERS += \
        tst_http_parse.h

SOURCES += \
        main.cpp\
        ../../../src/Protocol/HTTP/HttpParser.cpp\
        ../../../src/Protocol/HTTP/HttpRequest.cpp\
        ../../../src/Network/Buffer.cpp\
        ../../../src/Network/FD.cpp\
        ../../../src/Network/Timer.cpp\
        ../../../src/Network/TimerHeap.cpp\
        ../../../src/Network/Connection.cpp\
        ../../../src/Network/EventLoop.cpp\
        ../../../src/Network/EventPoller.cpp \
        ../../../src/Util/Util.cpp\
        ../../../src/Util/Clock.cpp\
        ../../../src/Util/Mutex.cpp\
        ../../../src/3rd/NanoLog/NanoLog.cpp\
