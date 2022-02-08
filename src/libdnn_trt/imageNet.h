 
#ifndef __IMAGE_NET_H__
#define __IMAGE_NET_H__

#include "tensorNet.h"
#include "dnn_types.h"

namespace trt {
	class imageNet : public tensorNet
	{
	public:
		static imageNet* create(const char *dataDir, const char *proto, const char *model, const char *mean,
			const char *label, const char *input, const char *output, int w, int h);

		virtual ~imageNet();

		//!
		//! \brief Runs the TensorRT inference engine for this sample
		//!
		bool infer(cv::Mat &img, std::vector<DNN_Prediction> &preds, int N = 5);
		bool infer(cv::Mat &img, std::vector<float> &feats, int outIndex = 0);

	protected:
		imageNet(const Params& params);
		Params mParams; //!< The parameters for the sample.

		//!
		//! \brief Filters output detections and verify results
		//!
		bool verifyOutput(const trt::BufferManager& buffers, std::vector<DNN_Prediction> &preds, int N=5);
	};
}

#endif
