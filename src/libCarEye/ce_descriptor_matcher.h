// ce_descriptor_matcher.h
//

#ifndef __CAR_EYE_DESCRIPTOR_MATCHER_HPP__
#define __CAR_EYE_DESCRIPTOR_MATCHER_HPP__


#include "opencv2/features2d/features2d.hpp"

namespace ce {

	class DescriptorMatcher 
	{
	public:
		DescriptorMatcher();
		~DescriptorMatcher();

		std::vector<cv::DMatch> match(const cv::Mat &arr1, const cv::Mat &arr2);

	protected:
		cv::Ptr<cv::BFMatcher> matcher;
	};

}
#endif //__CAR_EYE_DESCRIPTOR_MATCHER_HPP__