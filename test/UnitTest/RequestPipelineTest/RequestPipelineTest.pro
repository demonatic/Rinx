include(gtest_dependency.pri)

include(../../../lib_dependency.pri)

TEMPLATE = app
CONFIG += console c++17
CONFIG -= app_bundle
CONFIG += thread
CONFIG -= qt

HEADERS += \
    tst_request_pipeline.h

SOURCES += \
        main.cpp\
        ../../../src/Protocol/HFSMParser.hpp\
        ../../../src/Protocol/HTTP/HttpParser.cpp\
        ../../../src/Protocol/HTTP/HttpRequest.cpp\
        ../../../src/Network/Buffer.cpp\
        ../../../src/Network/Socket.cpp\
        ../../../src/Util/Util.cpp\
