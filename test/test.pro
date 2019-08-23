TEMPLATE=subdirs

CONFIG(debug,debug|release){
SUBDIRS += \
    UnitTest/BufferTest \
    UnitTest/HFSMParserTest\

}

SUBDIRS += \
    UnitTest/HttpParseTest \
    UnitTest/TimerTest
