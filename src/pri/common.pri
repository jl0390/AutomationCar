SolutionDir = ../..
BinDir=$$SolutionDir/bin
LibDir=$$SolutionDir/lib
IntDir=$$SolutionDir/obj
CONFIG += debug_and_release

DEFINES += USE_LOG \
        VIDEOIO_SHARED \
        CE_USE_STEREO_CALIB=1 \
        CE_USE_IMAGENET_FEATURE=1 \
        CE_USE_RANGE_FINDER=1

INCLUDEPATH += ../common ../utils

CONFIG(debug, debug|release) {
    TMP_DIR = $$IntDir/$$TARGET/debug
    QMAKE_CXXFLAGS += -std=gnu++0x -Wall
} else {
    TMP_DIR = $$IntDir/$$TARGET/release
    QMAKE_CXXFLAGS += -std=gnu++0x -Wall
}

OBJECTS_DIR = $$TMP_DIR/obj
MOC_DIR = $$TMP_DIR/moc
RCC_DIR = $$TMP_DIR/rcc
UI_DIR = $$TMP_DIR/ui

LIBS += -L"$$LibDir"
unix {
    LIBS += -L/usr/local/lib -L/usr/local/lib64 -L/usr/lib/aarch64-linux-gnu

    INCLUDEPATH += /usr/local/include \
		/usr/include/opencv4
}
