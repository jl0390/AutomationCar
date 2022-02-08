#ifndef __DENSE_FEATURE_DETECTOR_HPP__
#define __DENSE_FEATURE_DETECTOR_HPP__

#include "opencv2/core/core.hpp"
#include "opencv2/flann/miniflann.hpp"
#include "opencv2/features2d/features2d.hpp"

using namespace cv;
using namespace std;

namespace cv {

class DenseFeatureDetector : public FeatureDetector
{
public:
	explicit DenseFeatureDetector(float initFeatureScale = 1.f, int featureScaleLevels = 1,
		float featureScaleMul = 0.1f,
		int initXyStep = 6, int initImgBound = 0,
		bool varyXyStepWithScale = true,
		bool varyImgBoundWithScale = false);
	//AlgorithmInfo* info() const;

	void detect(const Mat& image, vector<KeyPoint>& keypoints, const Mat& mask = Mat()) const;

protected:

	double initFeatureScale;
	int featureScaleLevels;
	double featureScaleMul;

	int initXyStep;
	int initImgBound;

	bool varyXyStepWithScale;
	bool varyImgBoundWithScale;
};

}


#endif //__DENSE_FEATURE_DETECTOR_HPP__
