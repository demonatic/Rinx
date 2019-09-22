include(gtest_dependency.pri)

include(../../../lib_dependency.pri)

TEMPLATE = app
CONFIG += console c++17
CONFIG -= app_bundle
CONFIG += thread
CONFIG -= qt

HEADERS += \
        tst_hfsm_parser_test.h

SOURCES += \
        main.cpp\
        ../../../src/Protocol/HFSMParser.hpp
