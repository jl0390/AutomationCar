// ce_detector.h
//
#ifndef __CAR_EYE_DETECTOR_H__
#define __CAR_EYE_DETECTOR_H__

#include <string>
#include <opencv2/core/core.hpp>

#include "dnn_helper.h"
#include "ce_types.h"

namespace ce {
    class Detector
	{
	public:
        Detector(const char *dataDir, const char *prototxt, const char *weights, const char *label);
		~Detector();

		void setThreshold(double thresh) { m_threshold = thresh; }
        void setMinSize(int size) { m_minSize = size; }
        bool isOpened() { return m_net != NULL ? true : false; }
        pic::dnn::Detector* net() { return m_net; }

		int detect(cv::Mat &img, pic::dnn::DetectedObjList &objs);
		std::string labelName(int label);

	protected:
        double m_threshold;
        int m_minSize;
        pic::dnn::Detector* m_net;		//number detector

		int checkLabels(pic::dnn::DetectedObjList &objs);
	};
};

#endif //__CAR_EYE_DETECTOR_H__
