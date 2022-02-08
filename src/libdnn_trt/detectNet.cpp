#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

#include "detectNet.h"
#include "logger.h"

//#include "TimerChrono.h"

namespace trt {
//!
//! \brief Initializes members of the params struct using the command line args
//!
static void initializeDetectNetParams(detectNetParams &params, std::string modelName)
{
	params.outputClsSize = 91;
	params.calBatchSize = 10;
	params.nbCalBatches = 10;
	params.keepTopK = 100;

	if (DIMS_W(params.inputDims) == 0 || DIMS_H(params.inputDims) == 0) {
		DIMS_W(params.inputDims) = 300;
		DIMS_H(params.inputDims) = 300;
	}

	if (modelName.find("ssd_mobilenet_v1") == 0) {
		if (params.inputTensorName.empty())
			params.inputTensorName = "Input";
		if (params.outputTensorNames.size() == 0)
			params.outputTensorNames.push_back("Postprocessor");
	}
	else if (modelName.find("ssd_mobilenet_v2") == 0) {
		if (params.inputTensorName.empty())
			params.inputTensorName = "Input";
		if (params.outputTensorNames.size() == 0)
			params.outputTensorNames.push_back("NMS");
	}
}

detectNet* detectNet::create(const char *dataDir, const char *proto, const char *model, const char *mean,
	const char *label, const char *input, const char *output, int w, int h)
{
	detectNetParams params;
	initializeParams(params, dataDir, proto, model, mean, label, input, output, w, h);
	initializeDetectNetParams(params, model);

	detectNet *net = new detectNet(params);
	if (!net->init(params))
	{
		printf(LOG_TRT "detectNet -- failed to initialize.\n");
		return NULL;
	}
	return net;
}

detectNet::detectNet(const detectNetParams& params) : mParams(params)
{
}

detectNet::~detectNet()
{
}


static int checkIntersection(std::vector<DNN_DetectedObj> &objList, int batchSize)
{
	cv::Rect_<float> rc1, rc2;
	cv::Point2f pt1, pt2;
	std::vector<std::pair<int, int> > blocks;
	int start = 0;
	int pos=0;
	
	std::vector<std::vector<DNN_DetectedObj> >lists;
	lists.resize(batchSize);
	int listPos = 0;

	for (int i = 0; i < (int)objList.size(); i++) {
		DNN_DetectedObj &obj = objList[i];
		if (obj.label < 0) {
			listPos++;
			if (listPos >= batchSize)
				break;
		}
		else {
			lists[listPos].push_back(obj);
		}
	}

	for (int listPos=0;listPos<(int)lists.size(); listPos++) {
		std::vector<DNN_DetectedObj> &objs = lists[listPos];
		for (int i = 0; i < (int)objs.size() - 1; i++) {
			DNN_DetectedObj &obj1 = objs[i];
			rc1 = cv::Rect_<float>(obj1.x, obj1.y, obj1.width, obj1.height);
			pt1.x = obj1.x + obj1.width / 2;
			pt1.y = obj1.y + obj1.height / 2;
			for (int j = i + 1; j < (int)objs.size(); j++) {
				DNN_DetectedObj &obj2 = objs[j];
				rc2 = cv::Rect_<float>(obj2.x, obj2.y, obj2.width, obj2.height);
				pt2.x = obj2.x + obj2.width / 2;
				pt2.y = obj2.y + obj2.height / 2;

				int rmIdx = -1;
				if (rc1.contains(pt2) || rc2.contains(pt1)) {
					if (obj1.score > obj2.score)
						rmIdx = j;
					else
						rmIdx = i;
					objs.erase(objs.begin() + rmIdx);
					j--;
					if (rmIdx == i) {
						i--;
						break;
					}
				}
			}
		}
	}

	objList.clear();
	for (int listPos = 0; listPos < (int)lists.size(); listPos++) {
		std::vector<DNN_DetectedObj> &objs = lists[listPos];
		for (int i = 0; i < (int)objs.size(); ++i)
			objList.push_back(objs[i]);

		if (listPos < lists.size() - 1) {
			DNN_DetectedObj obj;
			memset(&obj, 0, sizeof(obj));
			obj.label = -1;
			objList.push_back(obj);
		}
	}

	return (int)objList.size();
}

//!
//! \brief Runs the TensorRT inference engine for this sample
//!
//! \details This function is the main execution function of the sample. It allocates the buffer,
//!          sets inputs and executes the engine.
//!
bool detectNet::infer(cv::Mat &img, std::vector<DNN_DetectedObj> &dets, float threshold)
{
	// Read the input data into the managed buffers
	assert(!mParams.inputTensorName.empty());

	mFrameDims.nbDims = 3;
	mFrameDims.d[0] = img.channels();
	mFrameDims.d[1] = img.rows;
	mFrameDims.d[2] = img.cols / mParams.batchSize;

	if (!processInput(*mBuffers, img))
	{
		return false;
	}

	// Memcpy from host input buffers to device input buffers
	mBuffers->copyInputToDevice();

	bool status = mContext->execute(mParams.batchSize, mBuffers->getDeviceBindings().data());
	if (!status)
		return false;

	// Memcpy from device output buffers to host output buffers
	mBuffers->copyOutputToHost();

	// Post-process detections and verify results
	if (!verifyOutput(*mBuffers, dets, threshold))
	{
		return false;
	}

	checkIntersection(dets, mParams.batchSize);

	return true;
}

//!
//! \brief Filters output detections and verify result
//!
//! \return whether the detection output matches expectations
//!
bool detectNet::verifyOutput(const trt::BufferManager& buffers, std::vector<DNN_DetectedObj> &dets, float threshold)
{
	const int inputH = mFrameDims.d[1];
	const int inputW = mFrameDims.d[2];
	const int batchSize = mParams.batchSize;
	const int keepTopK = mParams.keepTopK;
	const float visualThreshold = (threshold>0.1) ? threshold : mParams.visualThreshold;
	const int outputClsSize = mParams.outputClsSize;

	const float* detectionOut = static_cast<const float*>(buffers.getHostBuffer(mOutputBlobNames[0]));
	const int* keepCount = static_cast<const int*>(buffers.getHostBuffer(mOutputBlobNames[1]));
	DNN_DetectedObj obj;

	for (int p = 0; p < batchSize; ++p)
	{
		int numDetections = 0;
		// at least one correct detection
		bool correctDetection = false;

		for (int i = 0; i < keepCount[p]; ++i)
		{
			const float* det = &detectionOut[0] + (p * keepTopK + i) * 7;
			if (det[2] < visualThreshold)
			{
				continue;
			}

			// Output format for each detection is stored in the below order
			// [image_id, label, confidence, xmin, ymin, xmax, ymax]
			int detection = (int)det[1];
			assert(detection < outputClsSize);

			numDetections++;

			obj.label = (int)det[1];
			obj.score = det[2];

			obj.x = det[3] * inputW;
			obj.y = det[4] * inputH;
			obj.width = det[5] * inputW - obj.x;
			obj.height = det[6] * inputH - obj.y;

			dets.push_back(obj);
		}
		if (p < batchSize - 1) {
			memset(&obj, 0, sizeof(obj));
			obj.label = -1;
			dets.push_back(obj);
		}
	}

	return true;
}
}
