#include <qdebug.h>

#include "careyethread.h"

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <iostream>
#include <clocale>
#include <locale>
#include <codecvt>

#ifdef WIN32
	#include <direct.h>
#else
	#include <sys/stat.h>
    #include <sys/types.h>
    #define _mkdir(a) { umask(0); mkdir((a), 0755); }
#endif

#include "careyeapp.h"
#include "pstdthread.h"
#include "binocs_camera.h"
#include "videoio/video_writer.h"
#include "ce_infer.h"
#include "appconfig.h"

typedef struct DetectData {
    int64 totalTick;
    int64 plateTick;

    int detectCount;
    int correct;
    int incorrect;
    std::string compName;
    bool compRes;
} DetectData;

static DetectData s_data;

static BinocsCamera* createBinosCamera(const ce::Config &cfg)
{
    BinocsCamera* bc;
    if (strlen(cfg.video_file) > 0) {
        bc = new BinocsCamera(cfg.video_file);
        printf("Video file: %s\n", cfg.video_file);
    }
    else {
        bc = new BinocsCamera(cfg.camInfo.device, cfg.camInfo.width, cfg.camInfo.height);
        printf("Camera device=%d\n", cfg.camInfo.device);
    }
    return bc;
}

static void copyStr(const QString &src, char *dst, int len)
{
    strncpy(dst, src.toStdString().c_str(), len);
}

static void getConfig(ce::Config &cfg)
{
    memset(&cfg, 0, sizeof(cfg));
    copyStr(g_conf->value(CONF_DETECT_MODE_DIR).toString(), cfg.detModel.path, CE_MAX_PATH);
    copyStr(g_conf->value(CONF_DETECT_PROTO).toString(), cfg.detModel.protoFile, CE_MAX_PATH);
    copyStr(g_conf->value(CONF_DETECT_MODEL).toString(), cfg.detModel.modelFile, CE_MAX_PATH);
    copyStr(g_conf->value(CONF_DETECT_LABEL).toString(), cfg.detModel.labelFile, CE_MAX_PATH);
    cfg.detModel.threshold = g_conf->value(CONF_DETECT_THRESHOLD).toFloat();

    cfg.camInfo.device = g_conf->value(CONF_CAMERA_DEV).toInt();
    cfg.camInfo.width = g_conf->value(CONF_CAMERA_WIDTH).toInt();
    cfg.camInfo.height = g_conf->value(CONF_CAMERA_HEIGHT).toInt();

#if CE_USE_IMAGENET_FEATURE
    copyStr(g_conf->value(CONF_CLASS_MODE_DIR).toString(), cfg.clsModel.path, CE_MAX_PATH);
    copyStr(g_conf->value(CONF_CLASS_PROTO).toString(), cfg.clsModel.protoFile, CE_MAX_PATH);
    copyStr(g_conf->value(CONF_CLASS_MODEL).toString(), cfg.clsModel.modelFile, CE_MAX_PATH);
    copyStr(g_conf->value(CONF_CLASS_LABEL).toString(), cfg.clsModel.labelFile, CE_MAX_PATH);
    cfg.clsModel.threshold = g_conf->value(CONF_CLASS_THRESHOLD).toFloat();
#endif
#if 0 && CE_USE_RANGE_FINDER
    cfg.camInfo.dist = g_conf->value(CONF_CAMERA_DIST).toDouble();
    cfg.camInfo.focus = g_conf->value(CONF_CAMERA_FOCUS).toDouble();
    cfg.camInfo.delta = g_conf->value(CONF_CAMERA_DELTA).toDouble();
#endif

    copyStr(g_conf->value(CONF_CAMERA_PARAM).toString(), cfg.camInfo.param, CE_MAX_PATH);

    copyStr(g_conf->value(CONF_RECORD_DIR).toString(), cfg.record_dir, CE_MAX_PATH);
    cfg.record_time = g_conf->value(CONF_RECORD_TIME).toInt();

    copyStr(g_conf->value(CONF_VIDEO_FILE).toString(), cfg.video_file, CE_MAX_PATH);

    qDebug() << "URL:" << g_conf->value(CONF_SERVER_URL).toString();
    qDebug() << "collision threshold:" << g_conf->value(CONF_COLLISION_THRESHOLD).toFloat();
    qDebug() << "sould dir:" << g_conf->value(CONF_COLLISION_SOUND_DIR).toString();
    qDebug() << "sound files:" << g_conf->value(CONF_COLLISION_SOUND_FILES).toString();
}

CarEyeThread::CarEyeThread(QObject *parent)
    : PQThread(parent)
{
    m_infer = NULL;
    m_writer = NULL;

    ce::Config cfg;
    getConfig(cfg);

    m_camera = createBinosCamera(cfg);

    if (strlen(cfg.record_dir) > 0 && cfg.record_time > 0) {
        _mkdir(cfg.record_dir);
        m_writer = new videoio::Writer(m_camera->cap(), cfg.record_dir, "ce", 25, cfg.record_time);
        m_writer->setCropRect(0, 0, BC_CAP_HEIGHT, BC_CAP_HEIGHT);
    }

    m_infer = new ce::Infer();
    m_infer->setDetectModel(cfg.detModel);
#if CE_USE_IMAGENET_FEATURE
    m_infer->setClassifyModel(cfg.clsModel);
#endif
#if CE_USE_RANGE_FINDER
    m_infer->setRangeFinder(cfg.camInfo);
#endif
    m_infer->setCamera(m_camera);
    m_infer->setCameraParam(cfg.camInfo.param);

    if (strlen(cfg.video_file) > 0)
        setCondEnable(true);

    m_warningManager = new WarningManager(this);
    connect(m_warningManager, &WarningManager::soundChanged, g_app, &CarEyeApp::playSound);
}

CarEyeThread::~CarEyeThread()
{
    if (m_writer)
        delete m_writer;
    if (m_infer)
        delete m_infer;
    if (m_camera)
        delete m_camera;
    if (m_warningManager)
        delete m_warningManager;
}

static void scale(ce::DetInfo &info, float rate)
{
    info.x = round(rate*info.x);
    info.y = round(rate*info.y);
    info.width = round(rate*info.width);
    info.height = round(rate*info.height);
}

bool CarEyeThread::getInferRes(CarEyeInferRes &inferRes, int sw, int sh)
{
    wake();

    std::lock_guard<std::mutex> locker(m_inferResMutex);

    if (m_inferRes.frame.empty())
        return false;

    if (inferRes.frameTime >= m_inferRes.frameTime)
        return false;

    inferRes.fps = m_inferRes.fps;
    inferRes.infers = m_inferRes.infers;
    inferRes.frameTime = m_inferRes.frameTime;

    cv::Mat &frame = m_inferRes.frame;
    float rate = 1.0;
    if (sw > 0 && sh > 0) {
        float ratex = (float)sw / frame.cols;
        float ratey = (float)sh / frame.rows;
        rate = std::min(ratex, ratey);
    }

    if (fabs(rate - 1.0f) > 0.001) {
        int w = (int)((float)frame.cols*rate);
        int h = (int)((float)frame.rows*rate);
        cv::resize(frame, inferRes.frame, cv::Size(w, h));
    }
    else {
        frame.copyTo(inferRes.frame);
    }

    for (int i = 0; i < (int)inferRes.infers.size(); i++) {
        ce::InferInfo &infer = inferRes.infers[i];
        scale(infer.left, rate);
#if CE_USE_RANGE_FINDER
        scale(infer.right, rate);
#endif
    }

    return true;
}

void updateInfers(ce::InferList &oldInfers, ce::InferList &newInfers, ce::InferList &resultInfers){
    
}

void CarEyeThread::run()
{
    qDebug() << "CarEyeThread::run start";
    m_quit = 0;

    //m_camera->setCaptureSize(1280, 720);
    if (!m_infer->initAll() || !m_warningManager->init()){
        qDebug() << "Infer::initAll error!!!";
        m_quit = 1;
        return;
    }

    m_camera->open();
    while (!quited() && (m_camera->state() < videoio::StatePlaying)) {
        msleep(30);
    }
    if (quited())
        return;
    if (m_camera->state() != videoio::StatePlaying) {
        qDebug() << "BinocsCap::open error!!!";
        return;
    }
    if (m_writer)
        m_writer->start();

    BinocsFrame frame;
    double fps = 0;

    int ret = 0;
    PStdTimer timer;

    qDebug() << "CarEyeThread::loop start";

    ce::InferList infers;
    m_warningManager->start();

    while (!quited()) {
        msleep(10);
        timer.tic();

        ret = m_camera->getFrame(frame);
        if (ret < 1)
            continue;
        if (quited())
            break;

        infers.clear();
        if (ret == 1) {
            m_infer->run(frame.frame[0], infers);
            timer.toc();
            fps = 1000.0 / (double)timer.elapsed();
        }
        else if (ret == 2) {
            m_infer->run(frame.frame[0], frame.frame[1], frame.time, infers);
            calcCollisionTime(infers);
            m_warningManager->checkCollisionTime(infers);

            timer.toc();
            fps = 1000.0 / (double)timer.elapsed();
        }
        if (quited())
            break;

#if 0
        if (ret > 0) {
            cv::imshow("frame", frame.frame[0]);
            cv::waitKey(10);
        }
#endif
        m_inferResMutex.lock();
        frame.frame[0].copyTo(m_inferRes.frame);
        m_inferRes.infers = infers;
        m_inferRes.fps = fps;
        m_inferRes.frameTime = frame.time;
        m_inferResMutex.unlock();

        if (condEnabled())
            waitCond();
    }

    if (m_writer)
        m_writer->stop();
    m_camera->close();
    m_warningManager->stop();
    qDebug() << "CarEyeThread::run end";
}

double CarEyeThread::calcCollisionTime(ce::InferList &infers)
{
    double minTime = 10000;
    for (int i = 0; i < (int)infers.size(); i++) {
        ce::InferInfo &infer = infers[i];
        if (infer.velocity < 0.01)
            infer.collisionTime = 0;
        else
            infer.collisionTime = infer.distance*3.6 / infer.velocity;
        if (infer.collisionTime > 0.01 && minTime>infer.collisionTime)
            minTime = infer.collisionTime;
    }
    return minTime;
}
