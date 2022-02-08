// ce_types.h
//
#ifndef __CAR_EYE_TYPES_H__
#define __CAR_EYE_TYPES_H__

#ifdef QT_CORE_LIB
#include <QtCore/qglobal.h>
	#ifdef LIBCE_EXPORTS
		#define LIBCE_API Q_DECL_EXPORT
	#else
		#define LIBCE_API Q_DECL_IMPORT
	#endif
#else
	#ifdef LIBCE_EXPORTS
		#define LIBCE_API __declspec(dllexport)
	#else
		#define LIBCE_API __declspec(dllimport)
	#endif
#endif

#include "dnn_helper.h"

#ifdef _MAX_PATH
#define CE_MAX_PATH		_MAX_PATH
#else
#define CE_MAX_PATH		DNN_MAX_PATH
#endif

#pragma pack(push, 4)    

typedef pic::dnn::ModelInfo CE_MODEL_INFO;

typedef struct CE_CAMERA_INFO
{
    int device;
#if CE_USE_IMAGENET_FEATURE
    double dist;    //cmm
    double focus;   //pixcel
    double delta;   //pixcel
#endif
    char param[CE_MAX_PATH]; // camera parameter file(.xml)
    int width;  //the width of camera resolution
    int height; //the height of camera resolution
} CE_CAMERA_INFO;

typedef struct CE_CONFIG {
public:
    CE_MODEL_INFO detModel;
#if CE_USE_IMAGENET_FEATURE
    CE_MODEL_INFO clsModel;
#endif
    CE_CAMERA_INFO camInfo;

    char record_dir[CE_MAX_PATH];   // record dir
    long record_time;               // record time (min)

    char video_file[CE_MAX_PATH];   // file path
} CE_CONFIG;

enum CE_OPT {
	CE_OPT_DEBUG = 0,
	CE_OPT_THRESHOLD,
	CE_OPT_IMG_SIZE,
	CE_OPT_MAX
};

#pragma pack(pop)

#endif //__CAR_EYE_TYPES_H__

