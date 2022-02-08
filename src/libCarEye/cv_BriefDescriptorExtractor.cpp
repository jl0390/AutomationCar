#include "cv_BriefDescriptorExtractor.hpp"

#include "opencv2/features2d.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/core/utility.hpp"
#include "opencv2/core/ocl.hpp"
#include "opencv2/core/hal/hal.hpp"

#include <algorithm>
#include <algorithm>
#include <vector>

#include <iostream>
#include <iomanip>

using namespace cv;

inline int smoothedSum(const Mat& sum, const KeyPoint& pt, int y, int x)
{
    static const int HALF_KERNEL = BriefDescriptorExtractor::KERNEL_SIZE / 2;

    int img_y = (int)(pt.pt.y + 0.5) + y;
    int img_x = (int)(pt.pt.x + 0.5) + x;
    return   sum.at<int>(img_y + HALF_KERNEL + 1, img_x + HALF_KERNEL + 1)
           - sum.at<int>(img_y + HALF_KERNEL + 1, img_x - HALF_KERNEL)
           - sum.at<int>(img_y - HALF_KERNEL, img_x + HALF_KERNEL + 1)
           + sum.at<int>(img_y - HALF_KERNEL, img_x - HALF_KERNEL);
}

static void pixelTests16(const Mat& sum, const std::vector<KeyPoint>& keypoints, Mat& descriptors)
{
    for (int i = 0; i < (int)keypoints.size(); ++i)
    {
        uchar* desc = descriptors.ptr(i);
        const KeyPoint& pt = keypoints[i];
#include "generated_16.i"
    }
}

static void pixelTests32(const Mat& sum, const std::vector<KeyPoint>& keypoints, Mat& descriptors)
{
    for (int i = 0; i < (int)keypoints.size(); ++i)
    {
        uchar* desc = descriptors.ptr(i);
        const KeyPoint& pt = keypoints[i];

#include "generated_32.i"
    }
}

static void pixelTests64(const Mat& sum, const std::vector<KeyPoint>& keypoints, Mat& descriptors)
{
    for (int i = 0; i < (int)keypoints.size(); ++i)
    {
        uchar* desc = descriptors.ptr(i);
        const KeyPoint& pt = keypoints[i];

#include "generated_64.i"
    }
}

namespace cv
{

BriefDescriptorExtractor::BriefDescriptorExtractor(int bytes) :
    bytes_(bytes), test_fn_(NULL)
{
    switch (bytes)
    {
        case 16:
            test_fn_ = pixelTests16;
            break;
        case 32:
            test_fn_ = pixelTests32;
            break;
        case 64:
            test_fn_ = pixelTests64;
            break;
        default:
            CV_Error(Error::StsBadArg, "bytes must be 16, 32, or 64");
    }
}

int BriefDescriptorExtractor::descriptorSize() const
{
    return bytes_;
}

int BriefDescriptorExtractor::descriptorType() const
{
    return CV_8UC1;
}

void BriefDescriptorExtractor::read( const FileNode& fn)
{
    int dSize = fn["descriptorSize"];
    switch (dSize)
    {
        case 16:
            test_fn_ = pixelTests16;
            break;
        case 32:
            test_fn_ = pixelTests32;
            break;
        case 64:
            test_fn_ = pixelTests64;
            break;
        default:
            CV_Error(Error::StsBadArg, "descriptorSize must be 16, 32, or 64");
    }
    bytes_ = dSize;
}

void BriefDescriptorExtractor::write( FileStorage& fs) const
{
    fs << "descriptorSize" << bytes_;
}

void BriefDescriptorExtractor::compute(const Mat& image, std::vector<KeyPoint>& keypoints, Mat& descriptors) const
{
    // Construct integral image for fast smoothing (box filter)
    Mat sum;

    Mat grayImage = image;
    //if( image.type() != CV_8U ) cvtColor( image, grayImage, CV_BGR2GRAY );
	if (image.type() != CV_8U) cvtColor(image, grayImage, COLOR_BGR2GRAY);

    ///TODO allow the user to pass in a precomputed integral image
    //if(image.type() == CV_32S)
    //  sum = image;
    //else

    integral( grayImage, sum, CV_32S);

    //Remove keypoints very close to the border
    KeyPointsFilter::runByImageBorder(keypoints, image.size(), PATCH_SIZE/2 + KERNEL_SIZE/2);

    descriptors = Mat::zeros((int)keypoints.size(), bytes_, CV_8U);
    test_fn_(sum, keypoints, descriptors);
}

} // namespace cv
