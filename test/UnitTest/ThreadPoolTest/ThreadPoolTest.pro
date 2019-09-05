include(gtest_dependency.pri)

include(../../../lib_dependency.pri)

TEMPLATE = app
CONFIG += console c++17
CONFIG -= app_bundle
CONFIG += thread
CONFIG -= qt

INCLUDEPATH*=../../../src/

HEADERS += \
        tst_threadpool.h

SOURCES += \
        main.cpp\
        ../../../src/Util/ThreadPool.cpp\
        ../../../src/Util/Mutex.cpp\
