// ce_feature_detector.hpp
//

#ifndef __CAR_EYE_FEATURE_DETECTOR_HPP__
#define __CAR_EYE_FEATURE_DETECTOR_HPP__

#include "opencv2/core/core.hpp"
#include "opencv2/features2d/features2d.hpp"

#define USE_DenseFeatureDetector    0

#if USE_DenseFeatureDetector
#include "cv_DenseFeatureDetector.hpp"
#endif

#if USE_DenseFeatureDetector
typedef cv::DenseFeatureDetector    FeaturesDetector;
#else
typedef cv::FastFeatureDetector     FeaturesDetector;
#endif

#include "cv_BriefDescriptorExtractor.hpp"

namespace ce {

	class FeatureDetector
	{
	public:
		FeatureDetector();
		~FeatureDetector();

		void detectFeatures(const cv::Mat &img, std::vector<cv::KeyPoint> &keypnts);
		void computeDescripts(const cv::Mat &img, std::vector<cv::KeyPoint> &keypnts, cv::Mat &desc);

	protected:
        cv::Ptr<FeaturesDetector> m_featDetector;
		cv::Ptr<cv::BriefDescriptorExtractor> m_descExtractor;

	};
}

#endif //__CAR_EYE_FEATURE_DETECTOR_HPP__