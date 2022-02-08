#ifndef CAREYETHREAD_H
#define CAREYETHREAD_H

#include "pqthread.h"
#include "binocs_camera.h"
#include "videoio/video_writer.h"
#include "ce_infer.h"
#include "warningmanager.h"

class CarEyeInferRes
{
public:
    CarEyeInferRes() {
        fps = 0;
        frameTime = 0;
    }
    float fps;
    std::vector<ce::InferInfo> infers;
    qint64 frameTime;
    cv::Mat frame;
};

class CarEyeThread : public PQThread
{
    Q_OBJECT

public:
    CarEyeThread(QObject *parent = 0);
    ~CarEyeThread();

    bool getInferRes(CarEyeInferRes &inferRes, int sw=0, int sh=0);
protected:
    virtual void run();
    //return minimum time
    double calcCollisionTime(ce::InferList &infers);

private:
    BinocsCamera *m_camera;
    videoio::Writer *m_writer;
    ce::Infer *m_infer;

    std::mutex m_inferResMutex;
    CarEyeInferRes m_inferRes;

    WarningManager *m_warningManager;
};

#endif // CAREYETHREAD_H
