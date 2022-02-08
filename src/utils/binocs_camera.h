// ce_binocs_cap.h
//
#ifndef __CAR_EYE_BINOCS_CAP_H__
#define __CAR_EYE_BINOCS_CAP_H__

#include <string>
#include <vector>
#include <opencv2/core.hpp>

#include "videoio/video_cap.h"

#define BC_CAMERAS      2   //0:left, 1:right
#if 1
#define BC_CAP_2WIDTH   1280
#define BC_CAP_WIDTH    640
#define BC_CAP_HEIGHT   480
#else
#define BC_CAP_2WIDTH   (320*2)
#define BC_CAP_WIDTH    320
#define BC_CAP_HEIGHT   240
#endif

typedef struct CameraParam {
    CameraParam() {
        FT = 3000;
    }
    cv::Mat M1, M2, D1, D2, R, T, E, F, P1, R1, P2, R2, Q;
    double FT;

    cv::Rect roiRect1, roiRect2;
} CameraParam;

typedef struct CameraCalibData {
    cv::Mat img1, img2;
    cv::Size imageSize;
    cv::Mat map11, map12, map21, map22;
    cv::Mat img1r, img2r;
    cv::Mat pair, orgPair;
    cv::Mat disp, vdisp;
} CameraCalibData;

class BinocsFrame : public videoio::Frame {
public:
    void copyTo(BinocsFrame &dst) {
        for (int i = 0; i < BC_CAMERAS; i++) {
            if (frame[i].empty())
                dst.frame[i].release();
            else
                frame[i].copyTo(dst.frame[i]);
        }
        dst.number = number;
        dst.time = time;
    }

    cv::Mat frame[BC_CAMERAS];
};

class BinocsCameraPrivate;

class LIB_VIDEOIO_API BinocsCamera
{
public:
    BinocsCamera(int devId=0, int width=640, int height=480);
    BinocsCamera(const char* video_file);
    ~BinocsCamera();

    bool setParam(const std::string &filename);
    void setCaptureSize(int width, int height);
    videoio::Capture* cap();
    bool open();
    void close();
    int state();
    int getFrame(BinocsFrame &frame, bool ignoreTime = false);

    //stereo calibration
    void initRectifyMap(cv::Size size);
    bool remap(cv::Mat &leftImg, cv::Mat &rightImg, bool pair = false, bool grid = true);
    const cv::Mat* pair();
    bool calcDistances(cv::Mat &leftImg, cv::Mat &rightImg, const std::vector<cv::Rect2f> &rects,
        const std::vector<cv::KeyPoint> &keyPoints, std::vector<double> &distances);
    double calcDistance(const std::vector<cv::KeyPoint> &leftKeyPoints,
        const std::vector<cv::KeyPoint> &rightKeyPoints, const std::vector<cv::DMatch> &matchs, int width, int height);

protected:
    void construct();
    void computeDisparity();
    double calcDistance(const std::vector<cv::Point2f> &points);
    void createPair(bool grid, bool org=false);

private:
    BinocsCameraPrivate *m_d;
};

#endif //__CAR_EYE_BINOCS_CAP_H__
