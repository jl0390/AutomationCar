QT       -= gui
win32: TARGET = libcareye
unix: TARGET = careye
TEMPLATE = lib

DEFINES += LIBCE_EXPORTS

SOURCES += ce_binocular_tracker.cpp \
    ce_classifier.cpp \
    ce_descriptor_matcher.cpp \
    ce_detector.cpp \
    ce_feature_detector.cpp \
    ce_infer.cpp \
    ce_range_finder.cpp \
    cv_BriefDescriptorExtractor.cpp \
    cv_DenseFeatureDetector.cpp \
    ../utils/binocs_camera.cpp \
    ../utils/videoio/video_cap.cpp \
    ../utils/videoio/video_writer.cpp


HEADERS += ce_binocular_tracker.h \
    ce_classifier.h \
    ce_descriptor_matcher.h \
    ce_detector.h \
    ce_feature_detector.h \
    ce_infer.h \
    ce_math.hpp \
    ce_object_types.hpp \
    ce_range_finder.h \
    cv_BriefDescriptorExtractor.hpp \
    cv_DenseFeatureDetector.hpp \
    ../utils/binocs_camera.h \
    ../utils/videoio/video_cap.h \
    ../utils/videoio/video_writer.h \
    ../common/common.h

INCLUDEPATH += include \
    ../libdnn_trt/include \
    ../utils

include(../pri/common.pri)
include(../pri/opencv.pri)

DESTDIR = $$LibDir

unix {
    LIBS += -ldnn_trt
    target.path = /usr/local/lib
    INSTALLS += target
}
