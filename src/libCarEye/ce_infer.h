// ce_infer.h
//

#ifndef __CAR_EYE_INFER_H__
#define __CAR_EYE_INFER_H__

#include <opencv2/core.hpp>
#include <vector>

#include "dnn_helper.h"
#include "ce_types.h"
#include "ce_object_types.hpp"
#include "binocs_camera.h"

namespace ce
{
#pragma pack(push, 4)
    typedef CE_CONFIG   Config;

	typedef struct tDetInfo {
		int x, y, width, height;
#if CE_DEBUG
        cv::Mat image;
        std::vector<cv::KeyPoint> keyPoints;
#endif
	} DetInfo;

	typedef struct tInferInfo {
        id_t id = 0;
		DetInfo left;
#if CE_USE_RANGE_FINDER
        DetInfo right;
#endif
        std::string name;
        double distance = 0;        //m
        double velocity = 0;        //km/h

        double collisionTime = 0;   //second
	} InferInfo;
    typedef std::vector<InferInfo> InferList;

#pragma pack(pop)

    class InferPrivate;
	class LIBCE_API Infer
	{
	public:
		Infer();
		~Infer();

        void setDetectModel(const CE_MODEL_INFO &model);
#if CE_USE_IMAGENET_FEATURE
        void setClassifyModel(const CE_MODEL_INFO &model);
#endif
#if CE_USE_RANGE_FINDER
        void setRangeFinder(const CE_CAMERA_INFO &camInfo);
#endif
        void setCamera(BinocsCamera *cam);
        void setCameraParam(const std::string &filename);

        bool initAll();

        int run(cv::Mat &left_frame, cv::Mat &right_frame, int64 time, std::vector<InferInfo> &infers);
        int run(cv::Mat &frame, std::vector<InferInfo> &infers);

	protected:
        InferPrivate *m_d;

        bool initDetectModel(const char *dirName, const char *prototxt, const char *weights, const char *labelMap);
#if CE_USE_IMAGENET_FEATURE
        bool initClassifyModel(const char *dirName, const char *prototxt, const char *weights, const char *mean, const char *labelmap);
#endif
#if CE_USE_RANGE_FINDER
        bool initRangeFinder(double cam_dist, double cam_focus, double delta, double threshold = 0.5);
#endif
        bool initTracker();
        void extractFeatures(const cv::Mat &img, const pic::dnn::DetectedObjList &objs, std::vector<ce::MonoObj> &monos);
#if CE_USE_RANGE_FINDER
        int getFeatures(const cv::Mat &roi, std::vector<float> &feats);
#endif
        int appendSizeFeatures(int imgWidth, int imgHeight, int roiWidth, int roiHeight, std::vector<float> &feats);
        int createBinoObj(std::vector<ce::MonoObj>& monos, int64 time, std::vector<ce::BinoObj>& objs);
        void createInferList(BinoObjList &binoObjs, std::vector<InferInfo> &infers);
#if CE_USE_STEREO_CALIB
        bool initStereoCalib();
        void calcDistances(cv::Mat &left_img, cv::Mat &right_img, BinoObjList &objs);
#endif
    };
}


#endif //__CAR_EYE_INFER_H__
