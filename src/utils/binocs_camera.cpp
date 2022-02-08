#include <thread>
#include <mutex>
#include <list>
#include <map>

#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/features2d.hpp>
#include <opencv2/calib3d.hpp>

#if CV_MAJOR_VERSION >= 3
	#include <opencv2/videoio/videoio.hpp>
#endif

#include "opencv_ext.hpp"
#include "binocs_camera.h"
#include "pstdthread.h"

#define STEREO_MAX_DISPARITY    128
#define STEREO_SGBM             1

typedef struct BlockMatch {
#if STEREO_SGBM
    cv::Ptr<cv::StereoSGBM> bm;
#else
    cv::Ptr<cv::StereoBM> bm;
#endif
#if STEREO_3D
    cv::Mat disp_mem;
    cv::Mat img3d;
#endif
} BlockMatch;

class BinocsCameraPrivate {
public:
    BinocsCameraPrivate() {
        cap = NULL;
        width = 0;
        height = 0;
    }
    int width;
    int height;
    videoio::Capture *cap;

    CameraParam param;
    BlockMatch bm;
    CameraCalibData calibData;
};

BinocsCamera::BinocsCamera(int devId, int width, int height)
{
    m_d = new BinocsCameraPrivate;
    m_d->width = width ? width : BC_CAP_WIDTH;
    m_d->height = height ? height : BC_CAP_HEIGHT;
    m_d->cap = new videoio::Capture(devId);
    m_d->cap->setCaptureSize(width*2, height);

    construct();
}

BinocsCamera::BinocsCamera(const char *video_file)
{
    m_d = new BinocsCameraPrivate;
    m_d->cap = new videoio::Capture(video_file);
    construct();
}

BinocsCamera::~BinocsCamera()
{
    if (m_d) {
        if (m_d->cap)
            delete m_d->cap;
        delete m_d;
        m_d = NULL;
    }
}

void BinocsCamera::construct()
{
    //create stereo block match
    int disparity = STEREO_MAX_DISPARITY;
    int numDisparities = (disparity + 15) & -16;
    int cn = 1;

#if STEREO_SGBM
    int blockSize = 3;
    m_d->bm.bm = cv::StereoSGBM::create(
        1,                      //int minDisparity = 0
        numDisparities,         //int numDisparities = 16
        blockSize,              //int blockSize = 3
        8 * cn*blockSize*blockSize,//int P1 = 0
        32 * cn*blockSize*blockSize,//int P2 = 0
        32, //1,                      //int disp12MaxDiff = 0
        0,      //40,           //int preFilterCap = 0
        10,                     //int uniquenessRatio = 0
        0,      //100,                    //int speckleWindowSize = 0
        0,      //32,                     //int speckleRange = 0
        //cv::StereoSGBM::MODE_SGBM
        //cv::StereoSGBM::MODE_HH
        cv::StereoSGBM::MODE_SGBM_3WAY
        //cv::StereoSGBM::MODE_HH4
        );
#else
    int blockSize = 21;
    m_d->bm.bm = cv::StereoBM::create(numDisparities, blockSize);
#endif
}

void BinocsCamera::initRectifyMap(cv::Size size)
{
    if (m_d->calibData.imageSize.width == 0 || m_d->calibData.imageSize.height == 0) {
        int flags = cv::CALIB_ZERO_DISPARITY;
        m_d->calibData.imageSize = size;
        const CameraParam &param = m_d->param;
        cv::stereoRectify(m_d->param.M1, m_d->param.D1, m_d->param.M2, m_d->param.D2, m_d->calibData.imageSize,
            m_d->param.R, m_d->param.T, m_d->param.R1, m_d->param.R2, m_d->param.P1, m_d->param.P2, m_d->param.Q,
            flags, -1, m_d->calibData.imageSize, &m_d->param.roiRect1, &m_d->param.roiRect2);

        int m1type = CV_16SC2;  // CV_32FC1;
        cv::initUndistortRectifyMap(param.M1, param.D1, param.R1, param.P1, m_d->calibData.imageSize, m1type, m_d->calibData.map11, m_d->calibData.map12);
        cv::initUndistortRectifyMap(param.M2, param.D2, param.R2, param.P2, m_d->calibData.imageSize, m1type, m_d->calibData.map21, m_d->calibData.map22);
#if 0
        writeMapParam("map11.xml", "map", m_d->calibData.map11);
        writeMapParam("map12.xml", "map", m_d->calibData.map12);
        writeMapParam("map21.xml", "map", m_d->calibData.map21);
        writeMapParam("map22.xml", "map", m_d->calibData.map22);
#endif
    }
}

bool BinocsCamera::setParam(const std::string &filename)
{
    if (!m_d)
        return false;

    //load camera parameters
    try {
        cv::FileStorage fs(filename, cv::FileStorage::READ);

        if (fs.isOpened()) {
            fs["M1"] >> m_d->param.M1;
            fs["M2"] >> m_d->param.M2;
            fs["D1"] >> m_d->param.D1;
            fs["D2"] >> m_d->param.D2;
            fs["R"] >> m_d->param.R;
            fs["T"] >> m_d->param.T;
            fs["E"] >> m_d->param.E;
            fs["F"] >> m_d->param.F;

            fs["FT"] >> m_d->param.FT;
#if 0
            fs["P1"] >> m_d->param.P1;
            fs["R1"] >> m_d->param.R1;
            fs["P2"] >> m_d->param.P2;
            fs["R2"] >> m_d->param.R2;
            fs["Q"] >> m_d->param.Q;

            fs["roiRC1"] >> m_d->param.roiRect1;
            fs["roiRC2"] >> m_d->param.roiRect2;
#endif
        }
    }
    catch (...) {
        return false;
    }
    return true;
}

void BinocsCamera::setCaptureSize(int width, int height)
{
    if (m_d && m_d->cap)
        m_d->cap->setCaptureSize(width, height);
}

videoio::Capture* BinocsCamera::cap() { return m_d ? m_d->cap : NULL; }

bool BinocsCamera::open() {
    if (m_d && m_d->cap)
        return m_d->cap->open();
    return false;
}

void BinocsCamera::close() {
    if (m_d && m_d->cap)
        m_d->cap->close();
}

int BinocsCamera::state() {
    if (m_d && m_d->cap)
        return m_d->cap->state();
    return videoio::StateError;
}

static bool isBinocsFrame(cv::Size size)
{
    bool res = false;
    float rate = (float)size.width * 3 / size.height;
    if (rate - 8 > -0.00001)
        res = true;
    return res;
}

int BinocsCamera::getFrame(BinocsFrame &frame, bool ignoreTime)
{
    int ret = m_d->cap->getFrame(frame);

    if (ret <= 0)
        return ret;

    if (isBinocsFrame(frame.image.size())) {
        ret = 2;
        cv::Rect rc(0, 0, frame.image.cols / 2, frame.image.rows);
        frame.frame[0] = frame.image(rc);
        rc.x = frame.image.cols / 2;
        frame.frame[1] = frame.image(rc);
    }
    else {
        ret = 1;
        frame.frame[0] = frame.image;
        frame.frame[1].release();
    }
    return ret;
}

static void getRectPoints(const std::vector<cv::KeyPoint> &keyPoints, const std::vector<cv::Rect2f> &rects, std::vector<std::vector<cv::KeyPoint>> &points)
{
    points.resize(rects.size());
    for (size_t i = 0; i < keyPoints.size(); i++) {
        cv::Point2f pt;
        pt = keyPoints[i].pt;

        for (size_t ri = 0; ri < rects.size(); ri++) {
            cv::Rect2f rc = rects[ri];
            if (rc.contains(pt)) {
                points[ri].push_back(keyPoints[i]);
                break;
            }
        }
    }
}

static void getRectPoints(const std::vector<cv::KeyPoint> &keyPoints, const std::vector<cv::Rect2f> &rects, std::vector<std::vector<cv::Point2f>> &points)
{
    points.resize(rects.size());
    for (size_t i = 0; i < keyPoints.size(); i++) {
        cv::Point2f pt;
        pt = keyPoints[i].pt;

        for (size_t ri = 0; ri < rects.size(); ri++) {
            cv::Rect2f rc = rects[ri];
            if (rc.contains(pt)) {
                points[ri].push_back(pt);
                break;
            }
        }
    }
}

bool BinocsCamera::calcDistances(cv::Mat &leftImg, cv::Mat &rightImg, const std::vector<cv::Rect2f> &rects,
    const std::vector<cv::KeyPoint> &keyPoints, std::vector<double> &distances)
{
    if (!remap(leftImg, rightImg))
        return false;

    computeDisparity();

    distances.resize(rects.size());

    std::vector<std::vector<cv::Point2f>> points;
    getRectPoints(keyPoints, rects, points);

    for (size_t i = 0; i < points.size(); i++) {
        distances[i] = calcDistance(points[i]);
    }
    return true;
}

double BinocsCamera::calcDistance(const std::vector<cv::KeyPoint> &leftKeyPoints, 
    const std::vector<cv::KeyPoint> &rightKeyPoints, const std::vector<cv::DMatch> &matches, int width, int height)
{
    if (matches.size() == 0)
        return 0;

    initRectifyMap(cv::Size(width, height));

    std::vector<cv::Point2f> pts1, pts2;
    for (int i = 0; i < matches.size(); ++i) {
        cv::Point2f pt1 = leftKeyPoints[matches[i].queryIdx].pt;
        cv::Point2f pt2 = rightKeyPoints[matches[i].trainIdx].pt;
        pts1.push_back(pt1);
        pts2.push_back(pt2);
    }
    std::vector<cv::Point2f> undistorPoints1, undistorPoints2;
    cv::undistortPoints(pts1, undistorPoints1, m_d->param.M1, m_d->param.D1, m_d->param.R1, m_d->param.P1);
    cv::undistortPoints(pts2, undistorPoints2, m_d->param.M2, m_d->param.D2, m_d->param.R2, m_d->param.P2);

    double dist = 0, disparity = 0;
    std::map<int, int> counts;
    double roundDist = 10;

    std::map<int, int> counts2;
    double roundDist2 = 100;
    double dist2;

    for (size_t i = 0; i < undistorPoints1.size(); i++) {
        cv::Point pt1, pt2;
        pt1.x = (int)round(undistorPoints1[i].x);
        pt1.y = (int)round(undistorPoints1[i].y);
        if (!m_d->param.roiRect1.contains(pt1))
            continue;

        pt2.x = (int)round(undistorPoints2[i].x);
        pt2.y = (int)round(undistorPoints2[i].y);
        if (!m_d->param.roiRect2.contains(pt2))
            continue;

        if (abs(pt1.y - pt2.y) > 2)
            continue;

        disparity = round(pt1.x-pt2.x);
        if (m_d->param.FT > 0.1 && disparity > 0.9)
            dist = m_d->param.FT / disparity;
        else
            dist = 0;
        if (dist < 0.1)
            continue;

        dist = round(dist / roundDist) * roundDist;
        counts[(int)dist] ++;

        dist2 = round(dist / roundDist2) * roundDist2;
        counts2[(int)dist2] ++;
    }

    std::map<int, int>::iterator it;
    int maxcount = 0;
    for (it = counts2.begin(); it != counts2.end(); it++) {
        if (maxcount < it->second) {
            maxcount = it->second;
            dist2 = (double)it->first;
        }
    }

    maxcount = 0;
    for (it = counts.begin(); it != counts.end(); it++) {
        if (it->first >= dist2 - roundDist2 / 2 - roundDist / 2 && it->first <= dist2 + roundDist2 / 2 + roundDist / 2) {
            if (maxcount < it->second) {
                maxcount = it->second;
                dist = (double)it->first / 100;
            }
        }
    }
    if (maxcount < 2)
        dist = 0;
    return dist;
}

//
//protected
//
void BinocsCamera::computeDisparity()
{
    cv::Mat &img1 = m_d->calibData.img1r;
    cv::Mat &img2 = m_d->calibData.img2r;

    cv::Mat gray1, gray2;
    if (img1.channels() == 3)
        cv::cvtColor(img1, gray1, cv::COLOR_BGR2GRAY);
    else
        gray1 = img1;
    if (img2.channels() == 3)
        cv::cvtColor(img2, gray2, cv::COLOR_BGR2GRAY);
    else
        gray2 = img2;
    m_d->bm.bm->compute(gray1, gray2, m_d->calibData.disp);

#if CE_DEBUG
    cv::normalize(m_d->calibData.disp, m_d->calibData.vdisp, 0, 256, cv::NORM_MINMAX, CV_8U);
    cv::imshow("disparity", m_d->calibData.vdisp);
    cv::waitKey(1);
#endif
}

bool BinocsCamera::remap(cv::Mat &leftImg, cv::Mat &rightImg, bool pair, bool grid)
{
    if (leftImg.empty() || rightImg.empty())
        return false;
    if (leftImg.size() != rightImg.size())
        return false;

    m_d->calibData.img1 = leftImg;
    m_d->calibData.img2 = rightImg;

    initRectifyMap(m_d->calibData.img1.size());

    if (m_d->calibData.img1.size() != m_d->calibData.imageSize)
        cv::resize(m_d->calibData.img1, m_d->calibData.img1, m_d->calibData.imageSize);
    if (m_d->calibData.img2.size() != m_d->calibData.imageSize)
        cv::resize(m_d->calibData.img2, m_d->calibData.img2, m_d->calibData.imageSize);

    cv::remap(m_d->calibData.img1, m_d->calibData.img1r, m_d->calibData.map11, m_d->calibData.map12, cv::INTER_CUBIC);
    cv::remap(m_d->calibData.img2, m_d->calibData.img2r, m_d->calibData.map21, m_d->calibData.map22, cv::INTER_CUBIC);

    if (pair)
        createPair(grid);

#if CE_DEBUG
    if (!pair) {
        createPair(grid, true);
        if (!m_d->calibData.pair.empty()) {
            cv::imshow("pair", m_d->calibData.pair);
            //cv::imwrite("e:\\work\\p2.jpg", m_d->calibData.pair);
        }
        if (!m_d->calibData.orgPair.empty()) {
            cv::imshow("org pair", m_d->calibData.orgPair);
            //cv::imwrite("e:\\work\\p1.jpg", m_d->calibData.orgPair);
        }
        cv::waitKey(1);
    }
#endif
    return true;
}

const cv::Mat* BinocsCamera::pair()
{
    if (!m_d)
        return NULL;
    return &m_d->calibData.pair;
}

double BinocsCamera::calcDistance(const std::vector<cv::Point2f> &points)
{
    if (points.size() == 0)
        return 0;

    double dist = 0;
    double disparity = 0;
    std::vector<cv::Point2f> undistorPoints;
    cv::undistortPoints(points, undistorPoints, m_d->param.M1, m_d->param.D1, m_d->param.R1, m_d->param.P1);
    std::map<int, int> counts;
    double roundDist = 10;

    std::map<int, int> counts2;
    double roundDist2 = 100;
    double dist2;

    for (size_t i = 0; i < undistorPoints.size(); i++) {
        cv::Point pt;
        pt.x = (int)round(undistorPoints[i].x);
        pt.y = (int)round(undistorPoints[i].y);
        if (!m_d->param.roiRect1.contains(pt))
            continue;
        short *ptr = m_d->calibData.disp.ptr<short>(pt.y);
        disparity = (double)ptr[pt.x] / 16;
        if (m_d->param.FT > 0.1 && disparity > 0.9)
            dist = m_d->param.FT / disparity;
        else
            dist = 0;
        if (dist < 0.1)
            continue;

        dist = round(dist / roundDist) * roundDist;
        counts[(int)dist] ++;

        dist2 = round(dist / roundDist2) * roundDist2;
        counts2[(int)dist2] ++;
    }

    std::map<int, int>::iterator it;
    int maxcount = 0;
    for (it = counts2.begin(); it != counts2.end(); it++) {
        if (maxcount < it->second) {
            maxcount = it->second;
            dist2 = (double)it->first;
        }
    }

    maxcount = 0;
    for (it = counts.begin(); it != counts.end(); it++) {
        if (it->first >= dist2 - roundDist2 / 2 - roundDist / 2 && it->first <= dist2 + roundDist2 / 2 + roundDist / 2) {
            if (maxcount < it->second) {
                maxcount = it->second;
                dist = (double)it->first / 100;
            }
        }
    }
    if (maxcount < 2)
        dist = 0;
    return dist;
}

static void createPairImage(cv::Mat &img1, cv::Mat &img2, bool grid, cv::Mat &pair)
{
    int height = std::max(img1.rows, img2.rows);
    pair.create(height, img1.cols+img2.cols, CV_8UC3);

    cv::Mat part = pair.colRange(0, img1.cols);
    if (img1.channels() == 1)
        cv::cvtColor(img1, part, cv::COLOR_GRAY2BGR);
    else
        img1.copyTo(part);

    part = pair.colRange(img1.cols, img1.cols + img2.cols);
    if (img2.channels() == 1)
        cv::cvtColor(img2, part, cv::COLOR_GRAY2BGR);
    else
        img2.copyTo(part);

    if (grid) {
        cv::Scalar color(0, 255, 0);
        for (int j = 0; j < pair.rows; j += 40)
            cv::line(pair, cv::Point(0, j), cv::Point(pair.cols, j), color);
        for (int j = 0; j < pair.cols; j += 40)
            cv::line(pair, cv::Point(j, 0), cv::Point(j, pair.rows), color);
#if 0
        cv::Scalar color1(0, 0, 255);
        if (m_d->param.roiRect1.width <= 0 || m_d->param.roiRect1.height <= 0) {
            cv::rectangle(cd.pair, m_d->param.roiRect1, color1);
        }
        if (m_d->param.roiRect2.width<0 || m_d->param.roiRect2.height <= 0) {
            cv::Rect rc = m_d->param.roiRect2;
            rc.x += cd.imageSize.width;
            cv::rectangle(cd.pair, rc, color1);
        }
#endif
    }
}

void BinocsCamera::createPair(bool grid, bool org)
{
    CameraCalibData &cd = m_d->calibData;
    createPairImage(cd.img1r, cd.img2r, grid, cd.pair);
    if (org)
        createPairImage(cd.img1, cd.img2, grid, cd.orgPair);
}
