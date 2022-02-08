
#ifndef __TENSOR_NET_H__
#define __TENSOR_NET_H__

#include "NvInfer.h"

#include "NvCaffeParser.h"

#if NV_TENSORRT_MAJOR >= 5
#include "NvOnnxParser.h"
#include "NvUffParser.h"
#include "NvInferPlugin.h"
#endif

#include <memory>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>

#include "buffers.h"
//#include "common.h"

#define USE_CONFIG	1

namespace trt
{
#ifdef WIN32
#define strcasecmp	stricmp
#endif

#if NV_TENSORRT_MAJOR > 1
	typedef nvinfer1::DimsCHW Dims3;

#define DIMS_C(x) x.d[0]
#define DIMS_H(x) x.d[1]
#define DIMS_W(x) x.d[2]

#else
	typedef nvinfer1::Dims3 Dims3;

#define DIMS_C(x) x.c
#define DIMS_H(x) x.h
#define DIMS_W(x) x.w

#ifndef NV_TENSORRT_MAJOR
#define NV_TENSORRT_MAJOR 1
#define NV_TENSORRT_MINOR 0
#endif
#endif

	//!
	//! \brief The Params structure groups the basic parameters required by
	//!        all networks.
	//!
	struct Params
	{
		int batchSize{ 1 };                  //!< Number of inputs in a batch
		int dlaCore{ -1 };                   //!< Specify the DLA core to run network on.
		bool int8{ false };                  //!< Allow runnning the network in Int8 mode.
		bool fp16{ false };                  //!< Allow running the network in FP16 mode.
		std::vector<std::string> dataDirs;   //!< Directory paths where sample data files are stored
		std::string inputTensorName;
		std::vector<std::string> outputTensorNames;

		std::string modelFileName;		//!< Filename of a network
		std::string labelsFileName;

		Dims3 inputDims;
		float visualThreshold;			//!< The minimum score threshold to consider a detection

		//for caffe
		std::string prototxtFileName;	//!< Filename of prototxt design file of a network
		std::string meanFileName;		//!< Filename of mean file of a network
	};

	void initializeParams(Params &param, const char *dataDir, const char *proto, const char *model,
		const char *mean, const char *label, const char *input, const char *output, int w, int h);

	/**
	* Default maximum batch size
	* @ingroup tensorNet
	*/
#define DEFAULT_MAX_BATCH_SIZE  1

	/**
	* Prefix used for tagging printed log output from TensorRT.
	* @ingroup tensorNet
	*/
#define LOG_TRT "[TRT]   "


	/**
	* Enumeration for indicating the desired precision that
	* the network should run in, if available in hardware.
	* @ingroup tensorNet
	*/
	enum precisionType
	{
		TYPE_DISABLED = 0,	/**< Unknown, unspecified, or disabled type */
		TYPE_FASTEST,		/**< The fastest detected precision should be use (i.e. try INT8, then FP16, then FP32) */
		TYPE_FP32,		/**< 32-bit floating-point precision (FP32) */
		TYPE_FP16,		/**< 16-bit floating-point half precision (FP16) */
		TYPE_INT8,		/**< 8-bit integer precision (INT8) */
		NUM_PRECISIONS		/**< Number of precision types defined */
	};

	/**
	* Stringize function that returns precisionType in text.
	* @ingroup tensorNet
	*/
	const char* precisionTypeToStr(precisionType type);

	/**
	* Parse the precision type from a string.
	* @ingroup tensorNet
	*/
	precisionType precisionTypeFromStr(const char* str);

	/**
	* Enumeration for indicating the desired device that
	* the network should run on, if available in hardware.
	* @ingroup tensorNet
	*/
	enum deviceType
	{
		DEVICE_GPU = 0,			/**< GPU (if multiple GPUs are present, a specific GPU can be selected with cudaSetDevice() */
		DEVICE_DLA,				/**< Deep Learning Accelerator (DLA) Core 0 (only on Jetson Xavier) */
		DEVICE_DLA_0 = DEVICE_DLA,	/**< Deep Learning Accelerator (DLA) Core 0 (only on Jetson Xavier) */
		DEVICE_DLA_1,				/**< Deep Learning Accelerator (DLA) Core 1 (only on Jetson Xavier) */
		NUM_DEVICES				/**< Number of device types defined */
	};

	/**
	* Stringize function that returns deviceType in text.
	* @ingroup tensorNet
	*/
	const char* deviceTypeToStr(deviceType type);

	/**
	* Parse the device type from a string.
	* @ingroup tensorNet
	*/
	deviceType deviceTypeFromStr(const char* str);

	/**
	* Enumeration indicating the format of the model that's
	* imported in TensorRT (either caffe, ONNX, or UFF).
	* @ingroup tensorNet
	*/
	enum modelType
	{
		MODEL_CUSTOM = 0,	/**< Created directly with TensorRT API */
		MODEL_CAFFE,		/**< caffemodel */
		MODEL_ONNX,		/**< ONNX */
		MODEL_UFF			/**< UFF */
	};

	/**
	* Stringize function that returns modelType in text.
	* @ingroup tensorNet
	*/
	const char* modelTypeToStr(modelType type);

	/**
	* Parse the model format from a string.
	* @ingroup tensorNet
	*/
	modelType modelTypeFromStr(const char* str);

	/**
	* Profiling queries
	* @see tensorNet::GetProfilerTime()
	* @ingroup tensorNet
	*/
	enum profilerQuery
	{
		PROFILER_PREPROCESS = 0,
		PROFILER_NETWORK,
		PROFILER_POSTPROCESS,
		PROFILER_VISUALIZE,
		PROFILER_TOTAL,
	};

	/**
	* Stringize function that returns profilerQuery in text.
	* @ingroup tensorNet
	*/
	const char* profilerQueryToStr(profilerQuery query);

	/**
	* Profiler device
	* @ingroup tensorNet
	*/
	enum profilerDevice
	{
		PROFILER_CPU = 0,	/**< CPU walltime */
		PROFILER_CUDA,		/**< CUDA kernel time */
	};

	class tensorNet
	{
	public:
		template <typename T>
		using UniquePtr = std::unique_ptr<T, InferDeleter>;

		tensorNet();
		virtual ~tensorNet();

		//!
		//! \brief init the network engine
		//!
		bool init(const Params &params);

		void LoadLabel(const std::string &labelFileName);
		bool LoadNetwork(const Params &params);

		/**
		* Load a new network instance (this variant is used for UFF models)
		* @param prototxt File path to the deployable network prototxt
		* @param model File path to the caffemodel
		* @param mean File path to the mean value binary proto (NULL if none)
		* @param input_blob The name of the input blob data to the network.
		* @param input_dims The dimensions of the input blob (used for UFF).
		* @param output_blobs List of names of the output blobs from the network.
		* @param maxBatchSize The maximum batch size that the network will be optimized for.
		*/
		bool LoadNetwork(const char* prototxt, const char* model, const char* mean,
			const char* input_blob, const Dims3& input_dims,
			const std::vector<std::string>& output_blobs,
			uint32_t maxBatchSize = DEFAULT_MAX_BATCH_SIZE,
			precisionType precision = TYPE_FASTEST,
			deviceType device = DEVICE_GPU, bool allowGPUFallback = true,
			nvinfer1::IInt8Calibrator* calibrator = NULL, cudaStream_t stream = NULL);

		/**
		* Manually enable debug messages and synchronization.
		*/
		void EnableDebug();

		/**
		* Return true if GPU fallback is enabled.
		*/
		inline bool AllowGPUFallback() const { return mAllowGPUFallback; }

		/**
		* Retrieve the device being used for execution.
		*/
		inline deviceType GetDevice() const { return mDevice; }

		/**
		* Retrieve the type of precision being used.
		*/
		inline precisionType GetPrecision() const { return mPrecision; }

		/**
		* Check if a particular precision is being used.
		*/
		inline bool IsPrecision(precisionType type) const { return (mPrecision == type); }

		/**
		* Determine the fastest native precision on a device.
		*/
		static precisionType FindFastestPrecision(deviceType device = DEVICE_GPU, bool allowInt8 = true);

		/**
		* Detect the precisions supported natively on a device.
		*/
		static std::vector<precisionType> DetectNativePrecisions(deviceType device = DEVICE_GPU);

		/**
		* Detect if a particular precision is supported natively.
		*/
		static bool DetectNativePrecision(const std::vector<precisionType>& nativeTypes, precisionType type);

		/**
		* Detect if a particular precision is supported natively.
		*/
		static bool DetectNativePrecision(precisionType precision, deviceType device = DEVICE_GPU);

		//!
		//! \brief Cleans up any state created in the sample class
		//!
		bool teardown();
		const char * labelName(int label);
		nvinfer1::Dims inputDims() { return mInputDims; }

	protected:
		/**
		* Create and output an optimized network model
		* @note this function is automatically used by LoadNetwork, but also can
		*       be used individually to perform the network operations offline.
		* @param deployFile name for network prototxt
		* @param modelFile name for model
		* @param outputs network outputs
		* @param maxBatchSize maximum batch size
		* @param modelStream output model stream
		*/
		bool ProfileModel(const std::string& deployFile, const std::string& modelFile,
			const char* input, const Dims3& inputDims,
			const std::vector<std::string>& outputs, uint32_t maxBatchSize,
			precisionType precision, deviceType device, bool allowGPUFallback,
			nvinfer1::IInt8Calibrator* calibrator, std::ostream& modelStream);

		//!
		//! \brief Reads the input and mean data, preprocesses, and stores the result in a managed buffer
		//!
		bool processInput(const trt::BufferManager& buffers, cv::Mat &img);

	protected:
		/* Member Variables */
		std::string mPrototxtPath;
		std::string mModelPath;
		std::string mMeanPath;
		std::string mInputBlobName;
		std::vector<std::string> mOutputBlobNames;
		std::string mCacheEnginePath;
		std::string mCacheCalibrationPath;

		deviceType    mDevice;
		precisionType mPrecision;
		modelType     mModelType;

		nvinfer1::IRuntime* mInfer;
		nvinfer1::ICudaEngine* mEngine;
		nvinfer1::IExecutionContext* mContext;

		uint32_t mWidth;
		uint32_t mHeight;
		uint32_t mInputSize;
		uint32_t mMaxBatchSize;
		bool    mEnableDebug;
		bool	mAllowGPUFallback;

		nvinfer1::Dims mInputDims; //!< The dimensions of the input to the network.
		std::vector<nvinfer1::Dims> mOutputDims; //!< The dimensions of the output from the network.

		std::vector<std::string> mClasses;
		BufferManager *mBuffers;
	};
}
#endif
