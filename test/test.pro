TEMPLATE=subdirs

CONFIG(debug,debug|release){
SUBDIRS += \
    UnitTest/BufferTest \

}

#CONFIG += ordered
