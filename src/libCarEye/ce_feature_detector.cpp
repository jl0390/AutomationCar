// ce_feature_detector.hpp
//

#include "ce_feature_detector.h"

namespace ce {
	
FeatureDetector::FeatureDetector() {
#if USE_DenseFeatureDetector
    m_featDetector = new FeaturesDetector;
#else
    m_featDetector = FeaturesDetector::create();
#endif
	m_descExtractor = new cv::BriefDescriptorExtractor;
}

FeatureDetector::~FeatureDetector() {
	if (m_featDetector)
		m_featDetector.release();
	if (m_descExtractor)
		m_descExtractor.release();
}

void FeatureDetector::detectFeatures(const cv::Mat &img, std::vector<cv::KeyPoint> &keypnts) {
	keypnts.clear();
	m_featDetector->detect(img, keypnts);
}

void FeatureDetector::computeDescripts(const cv::Mat &img, std::vector<cv::KeyPoint> &keypnts, cv::Mat &desc) {
	m_descExtractor->compute(img, keypnts, desc);
}

}

