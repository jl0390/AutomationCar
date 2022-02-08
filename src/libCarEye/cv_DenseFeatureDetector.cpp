#include "cv_DenseFeatureDetector.hpp"


DenseFeatureDetector::DenseFeatureDetector(float _initFeatureScale, int _featureScaleLevels,
	float _featureScaleMul, int _initXyStep,
	int _initImgBound, bool _varyXyStepWithScale,
	bool _varyImgBoundWithScale) :
	initFeatureScale(_initFeatureScale), featureScaleLevels(_featureScaleLevels),
	featureScaleMul(_featureScaleMul), initXyStep(_initXyStep), initImgBound(_initImgBound),
	varyXyStepWithScale(_varyXyStepWithScale), varyImgBoundWithScale(_varyImgBoundWithScale)
{}


void DenseFeatureDetector::detect(const Mat& image, vector<KeyPoint>& keypoints, const Mat& mask) const
{
	float curScale = static_cast<float>(initFeatureScale);
	int curStep = initXyStep;
	int curBound = initImgBound;
	for (int curLevel = 0; curLevel < featureScaleLevels; curLevel++)
	{
		for (int x = curBound; x < image.cols - curBound; x += curStep)
		{
			for (int y = curBound; y < image.rows - curBound; y += curStep)
			{
				keypoints.push_back(KeyPoint(static_cast<float>(x), static_cast<float>(y), curScale));
			}
		}

		curScale = static_cast<float>(curScale * featureScaleMul);
		if (varyXyStepWithScale) curStep = static_cast<int>(curStep * featureScaleMul + 0.5f);
		if (varyImgBoundWithScale) curBound = static_cast<int>(curBound * featureScaleMul + 0.5f);
	}

	KeyPointsFilter::runByPixelsMask(keypoints, mask);
}
