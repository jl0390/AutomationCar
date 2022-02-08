// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the LIBDNN_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// LIBDNN_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifndef __LIBDNN_H__
#define __LIBDNN_H__

#ifdef QT_CORE_LIB
#include <QtCore/qglobal.h>

	#ifdef LIBDNN_EXPORTS
		#define LIBDNN_API Q_DECL_EXPORT
	#else
		#define LIBDNN_API Q_DECL_IMPORT
	#endif
#else
	#ifdef LIBDNN_EXPORTS
		#define LIBDNN_API __declspec(dllexport)
	#else
		#define LIBDNN_API __declspec(dllimport)
	#endif
#endif


#include <vector>
#include "dnn_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/*!
return the detector dnn_handle.
*/
LIBDNN_API dnn_handle dnn_createDetector(const char *dataDir, const char *proto, const char *model, const char *mean,
	const char *label, const char *input, const char *output, int w, int h);
LIBDNN_API void dnn_releaseDetector(dnn_handle h);
/*!
return 0 if succed.
*/
LIBDNN_API int dnn_getInputSize(dnn_handle h, int *width, int *height);
/*!
return the object count.
*/
LIBDNN_API int dnn_detect(dnn_handle h, int width, int height, int ch, unsigned char *data, int step,
	double threshold, DNN_DetectedObj *objs, int len);

LIBDNN_API const char* dnn_labelName(dnn_handle h, int label);

/*!
classification
*/
LIBDNN_API void* dnn_createClassifier(const char *dataDir, const char *proto, const char *model, const char *mean,
	const char *label, const char *input, const char *output, int w, int h);
LIBDNN_API void dnn_releaseClassifier(void *h);
/*!
return 0 if succed, -1 if error.
*/
LIBDNN_API int dnn_classify(void *h, int width, int height, int ch, unsigned char *data, int step, DNN_Prediction *objs, int len);
LIBDNN_API int dnn_getfeatures(void *h, int width, int height, int ch, unsigned char *data, int step, float *features, int len);

#ifdef __cplusplus
}
#endif
#endif //__LIBDNN_H__
