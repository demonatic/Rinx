include(gtest_dependency.pri)
include(../../../lib_dependency.pri)

TEMPLATE = app
CONFIG += console c++17
CONFIG -= app_bundle
CONFIG += thread
CONFIG -= qt

INCLUDEPATH*=../../../src/

HEADERS += \
    tst_buffer_io.h\

SOURCES += \
        main.cpp\
        ../../../src/Network/Buffer.cpp\
        ../../../src/Network/Socket.cpp\
        ../../../src/RinxDefines.cpp\
