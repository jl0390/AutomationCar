#include <iostream>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include "ce_classifier.h"

namespace ce {

	Classifier::Classifier(const char *dataDir, const char *prototxt, const char *weights, const char *mean, const char *label)
	{
		pic::dnn::ModelParams param = { prototxt, weights, mean, label};
		m_net = pic::dnn::Classifier::create(dataDir, param);
	}

	Classifier::~Classifier()
	{
		if (m_net)
			delete m_net;
		m_net = NULL;
	}

	int Classifier::classify(cv::Mat &img, pic::dnn::PredictionList &objs)
	{
		int res = m_net->classify(img, objs);
		//res = checkLabels(objs);
		return res;
	}

	int Classifier::getFeatures(cv::Mat &img, std::vector<float> &features)
    {
        int res = m_net->getfeature(img, features);
        return res;
    }
}
