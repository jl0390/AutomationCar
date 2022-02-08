// ce_classifier.h
//
#ifndef __CAR_EYE_CLASSIFIER_H__
#define __CAR_EYE_CLASSIFIER_H__

#include <string>
#include <opencv2/core/core.hpp>

#include "dnn_helper.h"
#include "ce_types.h"

namespace ce {
    class Classifier
	{
	public:
		Classifier(const char *dataDir, const char *prototxt, const char *weights, const char *mean, const char *label);
		~Classifier();

        void setThreshold(double thresh) { m_threshold = thresh; }
        bool isOpened() { return m_net != NULL ? true : false; }
        pic::dnn::Classifier* net() { return m_net; }

		int classify(cv::Mat &img, pic::dnn::PredictionList &objs);
		int getFeatures(cv::Mat &img, std::vector<float> &features);

	protected:
		double m_threshold;
		pic::dnn::Classifier *m_net;		//number detector
	};
};

#endif //__CAR_EYE_CLASSIFIER_H__
