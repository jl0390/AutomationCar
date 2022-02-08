// libssd.cpp : Defines the exported functions for the DLL application.
//

// Including SDKDDKVer.h defines the highest available Windows platform.

// If you wish to build your application for a previous Windows platform, include WinSDKVer.h and
// set the _WIN32_WINNT macro to the platform you wish to support before including SDKDDKVer.h.

#include <opencv2/imgproc.hpp>
#include "libdnn.h"
#include "detectNet.h"
#include "imageNet.h"

static cv::Mat makeRGBImage(int width, int height, int ch, unsigned char *data, int step)
{
	int type = CV_8UC1;
	if (ch == 3)
		type = CV_8UC3;
	else if (ch == 4)
		type = CV_8UC4;

	cv::Mat img(height, width, type, data, step);

	if (type == CV_8UC1)
		cv::cvtColor(img, img, cv::COLOR_GRAY2BGR);
	else if (type == CV_8UC4)
		cv::cvtColor(img, img, cv::COLOR_BGRA2BGR);

	return img;
}

LIBDNN_API dnn_handle dnn_createDetector(const char *dataDir, const char *proto, const char *model, const char *mean,
	const char *label, const char *input, const char *output, int w, int h)
{
	trt::detectNet* net = trt::detectNet::create(dataDir, proto, model, mean, label, input, output, w, h); 
	if (net) {
		nvinfer1::Dims dims = net->inputDims();
		cv::Size s(dims.d[2]*DNN_BATCHSIZE, dims.d[1]);
		cv::Mat img = cv::Mat::zeros(s, CV_8UC3);
		std::vector<DNN_DetectedObj> dets;
		net->infer(img, dets);
	}
	return (void *)net;
}

LIBDNN_API void dnn_releaseDetector(dnn_handle h)
{
	trt::detectNet *detector = (trt::detectNet *)h;
	if (detector)
		delete detector;
}

LIBDNN_API int dnn_getInputSize(dnn_handle h, int *width, int *height)
{
	trt::detectNet *detector = (trt::detectNet *)h;
	if (detector == NULL)
		return -1;

	nvinfer1::Dims dims = detector->inputDims();
	*width = dims.d[2];
	*height = dims.d[1];
	return 0;
}

LIBDNN_API int dnn_detect(dnn_handle h, int width, int height, int ch, unsigned char *data, int step,
	double threshold, DNN_DetectedObj *objs, int len)
{
	trt::detectNet *detector = (trt::detectNet *)h;
	if (detector == NULL)
		return 0;
	if (objs == NULL)
		return 0;

	cv::Mat img = makeRGBImage(width, height, ch, data, step);

	std::vector<DNN_DetectedObj> dets;
	if (!detector->infer(img, dets, (float)threshold))
		return 0;
		
	int count = (int)dets.size();
	if (count>0 && objs!=NULL && len>0) {
		count = std::min(count, len);
		for (int i = 0; i < count; ++i)
			objs[i] = dets[i];
	}

	return count;
}

LIBDNN_API void dnn_freeObjs(DNN_DetectedObj **objs)
{
	if (objs) {
		DNN_DetectedObj *ptr = *objs;
		if (ptr)
			delete[] ptr;
		*objs = NULL;
	}
}

LIBDNN_API void* dnn_createClassifier(const char *dataDir, const char *proto, const char *model, const char *mean,
	const char *label, const char *input, const char *output, int w, int h)
{
	trt::imageNet *net = trt::imageNet::create(dataDir, proto, model, mean, label, input, output, w, h);
	if (net) {
		cv::Mat img = cv::Mat::zeros(cv::Size(32, 32), CV_8UC3);
		std::vector<DNN_Prediction> preds;
		net->infer(img, preds);
	}
	return (void*)net;
}

LIBDNN_API void dnn_releaseClassifier(void *h)
{
	trt::imageNet *net = (trt::imageNet *)h;
	if (net)
		delete net;
}

LIBDNN_API int dnn_classify(void *h, int width, int height, int ch, unsigned char *data, int step, DNN_Prediction *pred, int len)
{
	trt::imageNet *net = (trt::imageNet *)h;
	if (!net)
		return -1;

	if (pred == NULL)
		return -1;

	cv::Mat img = makeRGBImage(width, height, ch, data, step);

	std::vector<DNN_Prediction> predictions;
	if (!net->infer(img, predictions, len))
		return 0;
	if (predictions.size() == 0)
		return 0;
	int count = std::min(len, (int)predictions.size());

	for (int i = 0; i < count; i++) {
		pred[i].label = predictions[i].label;
		pred[i].score = predictions[i].score;
	}
	return count;
}

LIBDNN_API int dnn_getfeatures(void *h, int width, int height, int ch, unsigned char *data, int step, float *features, int len)
{
	trt::imageNet *net = (trt::imageNet *)h;
	if (!net)
		return -1;

	if (features == NULL)
		return -1;

	cv::Mat img = makeRGBImage(width, height, ch, data, step);

	std::vector<float> feats;
	net->infer(img, feats);
	if (feats.size() == 0)
		return -1;
	int count = std::min(len, (int)feats.size());

	for (int i = 0; i < count; i++) {
		features[i] = feats[i];
	}
	return count;
}

LIBDNN_API const char* dnn_labelName(dnn_handle h, int label)
{
	trt::tensorNet *net = (trt::tensorNet*)h;
	if (!net)
		return NULL;
	return net->labelName(label);
}
