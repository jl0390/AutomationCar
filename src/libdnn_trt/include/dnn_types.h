#ifndef __DNN_TYPES_H__
#define __DNN_TYPES_H__

#define DNN_MAX_PATH        260

#define DNN_DETECT_COUNT	32
#define DNN_PREDICT_COUNT	5

#define DNN_THRESHOLD		0.1
#define DNN_BATCHSIZE		1

#define DNN_CLASS_THRESHOLD		0.9
#define DNN_CLASSIFIER_LABEL	0
#define DNN_MEAN_VALUE			"104,117,123"

//#define DNN_CLASS_FEATURES_NAME		"loss3/classifier"
#define DNN_CLASS_FEATURES_COUNT	1000

typedef void *	dnn_handle;

#pragma pack(push, 4)
	typedef struct _DNN_DetectedObj {
		float score;
		int label;
		float x;
		float y;
		float width;
		float height;
	} DNN_DetectedObj;

	typedef struct _DNN_Prediction {
		float score;
		int label;
	} DNN_Prediction;
#pragma pack(pop)

#endif //__DNN_TYPES_H__
