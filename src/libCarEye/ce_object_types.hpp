// ce_object_types.hpp
//

#ifndef __CAR_EYE_OBJECT_TYPES_HPP__
#define __CAR_EYE_OBJECT_TYPES_HPP__

#include "dnn_helper.h"
#include "opencv2/opencv.hpp"

namespace ce {
    typedef int64   id_t;

	typedef struct tMonoObj {
		int label = -1;
		float score = 0;
        int x = 0, y = 0, width = 0, height = 0;
		std::vector<float> features;
		std::vector<cv::KeyPoint> keypnts;
		cv::Mat descripts;
    } MonoObj;

    typedef std::vector<MonoObj> MonoObjList;

	typedef struct tBinoObj {
		id_t id = 0;
		MonoObj left;
#if CE_USE_RANGE_FINDER
        MonoObj right;
		std::vector<cv::DMatch> matches;
#endif
		double distance = 0;
		double velocity = 0;
		int64 frameTime = 0;
	} BinoObj;
    typedef std::vector<BinoObj> BinoObjList;
}
#endif //__CAR_EYE_OBJECT_TYPES_HPP__