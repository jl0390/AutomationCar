

unix: {
INCLUDEPATH += /usr/local/cuda/include \
	/usr/include/aarch64-linux-gnu

LIBS += -L/usr/local/cuda/lib64 \
	-L/usr/lib/aarch64-linux-gnu

LIBS += -lcudnn -lcublas -lcudart \
	-lnvinfer -lnvparsers -lnvinfer_plugin -lnvonnxparser
}
