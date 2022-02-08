#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

#include "imageNet.h"
#include "logger.h"

//#include "TimerChrono.h"

namespace trt {
static void initializeImageNetParams(Params &params, std::string modelName)
{
	if (DIMS_W(params.inputDims) == 0 || DIMS_H(params.inputDims) == 0) {
		DIMS_W(params.inputDims) = 224;
		DIMS_H(params.inputDims) = 224;
	}

	if (modelName.find("googlenet") != std::string::npos) {
		if (params.inputTensorName.empty())
			params.inputTensorName = "data";
		if (params.outputTensorNames.size() == 0) {
			params.outputTensorNames.push_back("loss3/classifier");
			params.outputTensorNames.push_back("prob");
		}
	}
	else if (modelName.find("mobilenet_v1") != std::string::npos) {
		if (params.inputTensorName.empty())
			params.inputTensorName = "input";
		if (params.outputTensorNames.size() == 0) {
			params.outputTensorNames.push_back("MobilenetV1/Predictions/Reshape");
			//params.outputTensorNames.push_back("MobilenetV1/Predictions/Softmax");
		}
	}
}

imageNet* imageNet::create(const char *dataDir, const char *proto, const char *model, const char *mean,
	const char *label, const char *input, const char *output, int w, int h)
{
	Params params;
	initializeParams(params, dataDir, proto, model, mean, label, input, output, w, h);
	initializeImageNetParams(params, model);

	imageNet *net = new imageNet(params);
	if (!net->init(params))
	{
		printf(LOG_TRT "imageNet -- failed to initialize.\n");
		return NULL;
	}
	return net;
}

imageNet::imageNet(const Params& params)
	: mParams(params)
{
}

imageNet::~imageNet()
{
}

//!
//! \brief Runs the TensorRT inference engine for this sample
//!
//! \details This function is the main execution function of the sample. It allocates the buffer,
//!          sets inputs and executes the engine.
//!
bool imageNet::infer(cv::Mat &img, std::vector<DNN_Prediction> &preds, int N)
{
	assert(!mParams.inputTensorName.empty());
	assert(mOutputDims.size() >= 1);

	if (!processInput(*mBuffers, img))
		return false;

    // Memcpy from host input buffers to device input buffers
    mBuffers->copyInputToDevice();

    bool status = mContext->execute(mParams.batchSize, mBuffers->getDeviceBindings().data());
	if (!status)
		return false;

    // Memcpy from device output buffers to host output buffers
    mBuffers->copyOutputToHost();

	// Post-process detections and verify results
	if (!verifyOutput(*mBuffers, preds, N))
		return false;

    return true;
}

bool imageNet::infer(cv::Mat &img, std::vector<float> &feats, int outIndex)
{
	assert(!mParams.inputTensorName.empty());
	assert(mOutputDims.size() >= 2);

	if (!processInput(*mBuffers, img))
		return false;

	// Memcpy from host input buffers to device input buffers
	mBuffers->copyInputToDevice();

	bool status = mContext->execute(mParams.batchSize, mBuffers->getDeviceBindings().data());
	if (!status)
		return false;

	// Memcpy from device output buffers to host output buffers
	mBuffers->copyOutputToHost();

	// Post-process detections and verify results
	int batchSize = mParams.batchSize;
	const float* hostBuff = static_cast<const float*>(mBuffers->getHostBuffer(mParams.outputTensorNames[outIndex]));
	if (!hostBuff)
		return false;

	int outClasses = mOutputDims[outIndex].d[0];
	if (outClasses==0)
		return false;

	//for (int p = 0; p < batchSize; ++p)
	{
		for (int i = 0; i < outClasses; ++i)
			feats.push_back(hostBuff[i]);
	}

	return true;
}

//!
//! \brief Filters output detections and verify result
//!
//! \return whether the detection output matches expectations
//!
bool imageNet::verifyOutput(const trt::BufferManager& buffers, std::vector<DNN_Prediction> &preds, int N)
{
	int batchSize = mParams.batchSize;
#if 0
	int outIdx = (int)mParams.outputTensorNames.size() - 1;
	const float* predOut = static_cast<const float*>(buffers.getHostBuffer(mParams.outputTensorNames[outIdx]));
#else
	int outIdx = (int)mOutputBlobNames.size() - 1;
	const float* predOut = static_cast<const float*>(buffers.getHostBuffer(mOutputBlobNames[outIdx]));
#endif
	int outClasses = mOutputDims[outIdx].d[0];
	if (outClasses == 1)
		outClasses = mOutputDims[outIdx].d[1];

	if (predOut == NULL)
		return false;

	//for (int p = 0; p < batchSize; ++p)
	{
		for (int i = 0; i < outClasses; ++i)
		{
			if (predOut[i] >= mParams.visualThreshold) {
				DNN_Prediction p;
				p.label = i;
				p.score = predOut[i];
				preds.push_back(p);
			}
		}
	}

	//sort
	for (int i = 0; i < (int)preds.size() - 1; ++i) {
		DNN_Prediction cur = preds[i];
		for (int j = i + 1; j < (int)preds.size(); ++j) {
			if (cur.score < preds[j].score) {
				cur = preds[j];
				preds[j] = preds[i];
				preds[i] = cur;
			}
		}
	}
	if (preds.size() > N)
		preds.resize(N);
	return true;
}

}
