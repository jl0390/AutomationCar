TEMPLATE = app
TARGET = careye

QT += core network multimedia

#CONFIG += console
CONFIG += link_pkgconfig

DEFINES += QT_DLL USE_MOBILE_DATA \
	CUDA_API_PER_THREAD_DEFAULT_STREAM USE_GUI=1 USE_GLFW=1 \
	USE_GLES=1 USE_OPENCV=1

OS_ARCH = aarch64

HEADERS += ../common/pstdthread.h \
	../common/pqthread.h \
	../utils/filesystem.h \
   	appconfig.h \
   	appconst.h \
   	careyeapp.h \
   	careyethread.h \
   	careyewnd.h \
    mplayerthread.h \
    warningmanager.h

SOURCES += ../utils/filesystem.cpp \
	appconfig.cpp \
	careyeapp.cpp \
	careyethread.cpp \
	careyewnd.cpp \
    mplayerthread.cpp \
    warningmanager.cpp

INCLUDEPATH += ../libdnn_trt/include \
	../libCarEye \
	../nvxio/include

include(../pri/common.pri)
include(../pri/opencv.pri)

DESTDIR = $$BinDir

unix {
    PKGCONFIG += xrandr xi xxf86vm x11
    PKGCONFIG += cudart-10.0
    PKGCONFIG += visionworks
    PKGCONFIG += opencv4

    LIBS += -L../libs/aarch64/linux/release \
	-L../3rdparty/freetype/libs \
	-L../3rdparty/glfw3/libs \
	-L/usr/lib/aarch64-linux-gnu/tegra-egl

    LIBS += -lcareye -ldnn_trt \
	../libs/aarch64/linux/release/libovx.a \
	../3rdparty/freetype/libs/libfreetype.a \
 	../3rdparty/glfw3/libs/libglfw3.a \
	/usr/lib/aarch64-linux-gnu/tegra/libcuda.so \
	/usr/lib/aarch64-linux-gnu/tegra-egl/libGLESv2_nvidia.so.2 \
	-lEGL

    target.path = /usr/local/bin
    INSTALLS += target
}

