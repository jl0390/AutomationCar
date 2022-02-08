#ifndef __BRIEF_DESCRIPTOR_EXTRACTOR_HPP__
#define __BRIEF_DESCRIPTOR_EXTRACTOR_HPP__


#include "opencv2/core/core.hpp"
#include "opencv2/flann/miniflann.hpp"
#include "opencv2/features2d/features2d.hpp"

using namespace cv;
using namespace std;

namespace cv {

class BriefDescriptorExtractor : public DescriptorExtractor
{
public:
	static const int PATCH_SIZE = 48;
	static const int KERNEL_SIZE = 9;

	// bytes is a length of descriptor in bytes. It can be equal 16, 32 or 64 bytes.
	BriefDescriptorExtractor(int bytes = 16);

	virtual void read(const FileNode&);
	virtual void write(FileStorage&) const;

	virtual int descriptorSize() const;
	virtual int descriptorType() const;

	/// @todo read and write for brief

	//AlgorithmInfo* info() const;

	virtual void compute(const Mat& image, vector<KeyPoint>& keypoints, Mat& descriptors) const;

protected:

	typedef void(*PixelTestFn)(const Mat&, const vector<KeyPoint>&, Mat&);

	int bytes_;
	PixelTestFn test_fn_;
};


}
#endif //__BRIEF_DESCRIPTOR_EXTRACTOR_HPP__
