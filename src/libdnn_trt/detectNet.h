 
#ifndef __DETECT_NET_H__
#define __DETECT_NET_H__

#include "tensorNet.h"
#include "dnn_types.h"

namespace trt {
//!
//! \brief The detectNetParams structure groups the additional parameters
//!
struct detectNetParams : public trt::Params
{
	int outputClsSize;          //!< The number of output classes
	int calBatchSize;           //!< The size of calibration batch
	int nbCalBatches;           //!< The number of batches for calibration
	int keepTopK;               //!< The maximum number of detection post-NMS
};

//! \brief  The detectNet class implements the SSD sample
//!
//! \details It creates the network using an UFF model
//!
class detectNet : public tensorNet
{
public:
	static detectNet* create(const char *dataDir, const char *proto, const char *model, const char *mean,
		const char *label, const char *input, const char *output, int w, int h);

	virtual ~detectNet();

	//!
	//! \brief Runs the TensorRT inference engine for this sample
	//!
	bool infer(cv::Mat &frame, std::vector<DNN_DetectedObj> &dets, float threshold=0);

protected:
	detectNet(const detectNetParams& params);
	detectNetParams mParams; //!< The parameters for the sample.

	nvinfer1::Dims mFrameDims;

	//!
	//! \brief Filters output detections and verify results
	//!
	bool verifyOutput(const trt::BufferManager& buffers, std::vector<DNN_DetectedObj> &dets, float threshold=0);

};
}

#endif
