// ssdhelper.h
//
#ifndef __DNN_HELPER_H__
#define __DNN_HELPER_H__

#include <vector>
#include "opencv2/core/core.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "libdnn.h"

namespace pic {
namespace dnn {

#pragma pack(push, 4)
	typedef struct Rate{
		float x;
		float y;
		float dx;
		float dy;
	} Rate;

    typedef struct ModelInfo
    {
        char path[DNN_MAX_PATH];
        char protoFile[DNN_MAX_PATH];
        char modelFile[DNN_MAX_PATH];
        char meanFile[DNN_MAX_PATH];
        char labelFile[DNN_MAX_PATH];
        float threshold;
    } ModelInfo;

#pragma pack(pop)

#define ZERO				0.00001

	typedef DNN_DetectedObj DetectedObj;
	typedef std::vector<DNN_DetectedObj> DetectedObjList;
	typedef DNN_Prediction Prediction;
	typedef std::vector<DNN_Prediction> PredictionList;

	typedef struct _ModelParams {
		std::string proto;
		std::string model;
		std::string mean;
		std::string label;
		std::string input;
		std::string output;
		int inWidth;
		int inHeight;
	} ModelParams;

	class Classifier
	{
	public:
		static Classifier* create(const char *dirName, const ModelParams &param) {
			Classifier* net = new Classifier();
			if (!net->init(dirName, param)) {
				delete net;
				net = NULL;
			}
			return net;
		}

		~Classifier() {
			if (m_net)
				dnn_releaseClassifier(m_net);
			m_net = NULL;
		}

		const char* label(int label) {
			if (!m_net)
				return NULL;
			return dnn_labelName(m_net, label);
		}

		int classify(cv::Mat &img, dnn::PredictionList &preds) {
			if (!m_net)
				return 0;

			preds.resize(DNN_PREDICT_COUNT);

			int count = dnn_classify(m_net, img.cols, img.rows, img.channels(), img.data, (int)img.step[0],
				preds.data(), DNN_PREDICT_COUNT);
			if (count < DNN_PREDICT_COUNT)
				preds.resize(count);
			return count;
		}

		int getfeature(cv::Mat &img, std::vector<float> &feat)
		{
			if (!m_net)
				return 0;

			feat.resize(DNN_CLASS_FEATURES_COUNT);
			int count = dnn_getfeatures(m_net, img.cols, img.rows, img.channels(), img.data, (int)img.step[0],
				feat.data(), DNN_CLASS_FEATURES_COUNT);
			if (count < DNN_CLASS_FEATURES_COUNT)
				feat.resize(count);
			return count;
		}

	protected:
		Classifier() {
			m_net = 0;
		}

		bool init(const char *dirName, const ModelParams &params) {
			m_net = dnn_createClassifier(dirName, params.proto.c_str(), params.model.c_str(), params.mean.c_str(),
				params.label.c_str(), params.input.c_str(), params.output.c_str(), params.inWidth, params.inHeight);
			if (!m_net)
				return false;
			return true;
		}

		dnn_handle m_net;
	};


	class Detector
	{
	public:
		static Detector* create(const char *dirName, const ModelParams &param) {
			Detector* net = new Detector();
			if (!net->init(dirName, param)) {
				delete net;
				net = NULL;
			}
			return net;
		}

		~Detector() {
			if (m_net)
				dnn_releaseDetector(m_net);
			m_net = NULL;
		}

		const char* label(int label) {
			if (!m_net)
				return NULL;
			return dnn_labelName(m_net, label);
		}

		int detect(cv::Mat &img, double threshold, DetectedObjList &objs)
		{
			if (m_net == NULL)
				return 0;
			objs.resize(DNN_DETECT_COUNT);

			int count = dnn_detect(m_net, img.cols, img.rows, img.channels(), img.data, (int)img.step[0],
				threshold, objs.data(), (int)objs.size());

			objs.resize(count);
			return count;
		}

	protected:
		Detector() {
			m_net = 0;
		}

		bool init(const char *dirName, const ModelParams &params) {
			m_net = dnn_createDetector(dirName, params.proto.c_str(), params.model.c_str(), params.mean.c_str(),
				params.label.c_str(), params.input.c_str(), params.output.c_str(), params.inWidth, params.inHeight);
			if (!m_net)
				return false;
			return true;
		}

		dnn_handle m_net;
	};

	//utilities for image
	static cv::Mat makeImage(int width, int height, int ch, unsigned char *data, int step)
	{
		cv::Mat img;

		int type = CV_8UC1;
		if (ch == 3)
			type = CV_8UC3;
		else if (ch == 4)
			type = CV_8UC4;
		img = cv::Mat(height, width, type, data, step);

		return img;
	}

	static float resizeImage(cv::Mat &src, cv::Mat &dst, int size)
	{
		float rate = 1.0f;
		int orgSize = std::max<int>(src.cols, src.rows);
		int newSize = (size > 0 || std::abs(size) < orgSize) ? std::abs(size) : orgSize;

		rate = (float)newSize / orgSize;
		if (fabs(rate-1.0)>ZERO) {
			int width = (int)(src.cols*rate+0.5);
			int height = (int)(src.rows*rate+0.5);

			cv::resize(src, dst, cv::Size(width, height));
		}
		else if (dst.data != src.data) {
			dst = src;
		}
		return rate;
	}

	static Rate makeSquareImage(cv::Mat &src, cv::Mat &dst, int size, int aspectRatio)
	{
		Rate r = { 1.0f, 1.0f };
		int orgSize = std::max(src.cols, src.rows);
		int newSize = (size > 0 || std::abs(size) < orgSize) ? std::abs(size) : orgSize;

		if (aspectRatio) {
			r.x = r.y = (float)newSize / orgSize;
		}
		else {
			r.x = (float)newSize / src.cols;
			r.y = (float)newSize / src.rows;
		}

		if (src.cols == newSize && src.rows == newSize) {
			if (dst.data != src.data)
				dst = src;
		}
		else {
			int w, h;
			w = (int)(r.x*src.cols + 0.5);
			h = (int)(r.y*src.rows + 0.5);

			cv::resize(src, dst, cv::Size(w, h));
			if (w != h) {
				int _width = std::max(w, h);
				int _height = _width;
#if 0
				r.dx = (float)(_width - w) / 2;
				r.dy = (float)(_height - h) / 2;
#endif
				cv::Mat tmp = cv::Mat(cv::Size(_width, _height), src.type(), cv::Scalar(0, 0, 0));
				dst.copyTo(tmp(cv::Rect((int)(r.dx+0.5), (int)(r.dy+0.5), w, h)));
				dst = tmp;
			}
		}
		return r;
	}

	template <typename T>
	static int copyTo(std::vector<T> &src, T *dst, int len) {
		if (src.size() == 0 || dst == NULL || len == 0)
			return 0;

		int count = std::min((int)src.size(), len);
		for (int i = 0; i < count; i++)
			dst[i] = src[i];

		return count;
	}

	static void rescale(DNN_DetectedObj &obj, float scale)
	{
		if (scale > ZERO && fabs(scale - 1.0) > ZERO) {
			obj.x = obj.x / scale;
			obj.y = obj.y / scale;
			obj.width = obj.width / scale;
			obj.height = obj.height / scale;
		}
	}
	static void rescale(DNN_DetectedObj &obj, dnn::Rate scale)
	{
		obj.x -= scale.dx;
		obj.y -= scale.dy;
		if (scale.x > ZERO && fabs(scale.x - 1.0)>ZERO) {
			obj.x = obj.x / scale.x;
			obj.width = obj.width / scale.x;
		}
		if (scale.y > ZERO && fabs(scale.y - 1.0) > ZERO) {
			obj.y = obj.y / scale.y;
			obj.height = obj.height / scale.y;
		}
	}

	template<typename T>
	static void rescale(dnn::DetectedObjList *objs, T scale)
	{
		for (int i = 0; i < (int)objs->size(); ++i)
			rescale(objs->at(i), scale);
	}

	template<typename T>
	static void rescale(dnn::DetectedObj *objs, int count, T scale)
	{
		for (int i = 0; i < count; ++i)
			rescale(objs[i], scale);
	}

	template<typename T>
	static void adjustRect(int width, int height, T &rc)
	{
		if (rc.x < 0) {
			rc.width += rc.x;
			rc.x = 0;
		}
		if (rc.y < 0) {
			rc.height += rc.y;
			rc.y = 0;
		}
		if (rc.x + rc.width>width) {
			rc.width = width - rc.x;
		}
		if (rc.y + rc.height>height) {
			rc.height = height - rc.y;
		}
	}
}
}
#endif //__DNN_TYPES_H__
