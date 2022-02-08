
win32: {
LIBCAFFE_DIR = $$SolutionDir/../3rdparty/caffe/$$Platform
CUDNN_DIR = $$SolutionDir)/../3rdparty/cudnn/cuda/lib/$$Platform
INCLUDEPATH += $$LIBCAFFE_DIR)/../include \
            $$CUDNN_DIR/../../include

LIBS += -L"$$LIBCAFFE_DIR" -L"$$CUDNN_DIR" \
        -llibcaffe -cudnn
}

unix: {
INCLUDEPATH += /usr/local/cuda/include
LIBS += -L/usr/local/cuda/lib64 \
	-L/usr/local/lib
LIBS += -lcaffe -lcudnn
}
