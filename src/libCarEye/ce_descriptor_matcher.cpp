#include "ce_descriptor_matcher.h"


namespace ce {

	DescriptorMatcher::DescriptorMatcher()
	{
		if (!matcher) {
			matcher = new cv::BFMatcher(cv::NORM_L1, true);
			//matcher = new cv::BFMatcher(cv::NORM_L2, true);
			//matcher = new cv::BFMatcher(cv::NORM_HAMMING, true);
			//matcher = new cv::BFMatcher(cv::NORM_HAMMING2, true);
			
		}
	}

	DescriptorMatcher::~DescriptorMatcher()
	{
		if (matcher)
			matcher.release();
	}

	std::vector<cv::DMatch> DescriptorMatcher::match(const cv::Mat &arr1, const cv::Mat &arr2)
	{
		std::vector<cv::DMatch> matches;
		if (arr1.rows == 0 || arr2.rows == 0)
			return matches;
		matcher->match(arr1, arr2, matches);
		return matches;
	}
}
