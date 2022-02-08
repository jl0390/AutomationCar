QT       -= gui
win32: TARGET = libdnn_trt
unix: TARGET = dnn_trt
TEMPLATE = lib

DEFINES += LIBDNN_EXPORTS USE_SSD_MOBILE

SOURCES += libdnn.cpp \
    tensorNet.cpp \
    imageNet.cpp \
    detectNet.cpp \
    common/logger.cpp \
    common/FlattenConcat.cpp

HEADERS += include/libdnn.h \
    include/dnn_types.h \
    include/dnn_helper.h \
    tensorNet.h \
    imageNet.h \
    detectNet.h \
    common/logger.h

INCLUDEPATH += include common \

include(../pri/common.pri)
include(../pri/tensorrt.pri)
include(../pri/opencv.pri)

DESTDIR = $$LibDir

unix {
    target.path = /usr/local/lib
    INSTALLS += target
}
