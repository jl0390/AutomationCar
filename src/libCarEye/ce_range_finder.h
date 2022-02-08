// ce_range_finder.h
//

#ifndef __CAR_EYE_RANGE_FINDER_H__
#define __CAR_EYE_RANGE_FINDER_H__

#include "ce_types.h"
#include "dnn_helper.h"
#include "ce_feature_detector.h"
#include "ce_descriptor_matcher.h"
#include "ce_object_types.hpp"

namespace ce {

	class RangeFinder
	{
	public:
		RangeFinder(double cam_dist, double cam_focus, double delta, double threshold = 0.5);
		~RangeFinder();

        int findObjs(const cv::Mat &left, const cv::Mat &right,
            std::vector<MonoObj> &leftobjs, std::vector<MonoObj> &rightobjs,
            std::vector<BinoObj> &bis);

		std::vector<BinoObj> computeDistance(const cv::Mat &left, const cv::Mat &right,
			std::vector<MonoObj> &leftobjs, std::vector<MonoObj> &rightobjs, int64 time = 0);


	protected:
		double m_threshold;
		double m_camDist, m_camFocus, m_delta;
		int m_divider;

		FeatureDetector *featDetector = NULL;
        std::vector<cv::KeyPoint> m_leftKeypnts, m_rightKeypnts;

        DescriptorMatcher *m_descMatcher = NULL;

		void extractFeatures(std::vector<MonoObj> &mos, const std::vector<cv::KeyPoint> &keypnts);
		void extractCenterFeatures(std::vector<MonoObj> &mos, const std::vector<cv::KeyPoint> &keypnts, int width, int height);
		std::vector<cv::KeyPoint> extractKeypoints(const std::vector<cv::KeyPoint> &keypnts, const cv::Rect &rect);
		void extractDescriptors(std::vector<MonoObj> &mos, const cv::Mat &img);
        int matchMonoObjs(const std::vector<MonoObj> &leftmos, const std::vector<MonoObj> &rightmos, std::vector<BinoObj> &bis);
		double calcDistances(const BinoObj &bi);
	};

}
#endif //__CAR_EYE_RANGE_FINDER_H__