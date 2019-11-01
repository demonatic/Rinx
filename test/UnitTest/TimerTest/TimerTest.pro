include(gtest_dependency.pri)

include(../../../lib_dependency.pri)

TEMPLATE = app
CONFIG += console c++17
CONFIG -= app_bundle
CONFIG += thread
CONFIG -= qt

INCLUDEPATH*=../../../src/

HEADERS += \
        tst_timer.h

SOURCES += \
        main.cpp\
        ../../../src/Network/Timer.cpp\
        ../../../src/Network/TimerHeap.cpp\
        ../../../src/Network/EventLoop.cpp\
        ../../../src/Network/EventPoller.cpp\
        ../../../src/Network/FD.cpp\
        ../../../src/Util/Clock.cpp\
        ../../../src/Util/Mutex.cpp\
        ../../../src/3rd/NanoLog/NanoLog.cpp\
