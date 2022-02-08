#include <thread>
#include <mutex>
#include <list>
#include <iostream>

#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#if CV_MAJOR_VERSION >= 3
#include <opencv2/videoio/videoio.hpp>
#endif
#include "opencv_ext.hpp"
#include "pstdthread.h"
#include "video_cap.h"


namespace videoio {
    class CapPrivate
    {
    public:
        CapPrivate() {
            abort = 0;
            state = 0;
            frameNum = 0;
            frameTime = 0;
            frameCond = -1;
            devId = -1;
        }
        cv::VideoCapture cap;
        int devId;
        std::string filename;
        float fps;
        std::mutex mutex;
        std::thread thread;
        int abort;
        int state;

        std::mutex frameMutex;
        cv::Mat frame;
        int64 frameNum;
        int64 frameTime;
        int frameCond;

        //config
        cv::Size capSize;
        cv::Size cropSize;

        //filters
        std::list<CapFilter *> filters;
    };

    static int aborted(CapPrivate *p) {
        std::lock_guard<std::mutex> lock(p->mutex);
        return p->abort;
    }

    Capture::Capture(int devId)
    {
        m_d = new CapPrivate();
        m_d->devId = devId;
    }

    Capture::Capture(const char *video_file)
    {
        m_d = new CapPrivate();
        m_d->filename = video_file;
    }

    Capture::~Capture()
    {
        close();
        if (m_d) {
            delete m_d;
            m_d = NULL;
        }
    }

    void Capture::setCaptureSize(int w, int h)
    {
        m_d->capSize.width = w;
        m_d->capSize.height = h;

        if (m_d->cap.isOpened() && m_d->capSize.width>0 && m_d->capSize.height>0) {
            m_d->cap.set(CV_CAP_PROP_FRAME_WIDTH, m_d->capSize.width);
            m_d->cap.set(CV_CAP_PROP_FRAME_HEIGHT, m_d->capSize.height);
            //m_d->cap.set(CV_CAP_PROP_FPS, 25);
        }
    }

    void Capture::setCropSize(int w, int h)
    {
        m_d->cropSize.width = w;
        m_d->cropSize.height = h;
    }

    static void setState(CapPrivate *p, int state) {
        std::lock_guard<std::mutex> lock(p->mutex);
        p->state = state;
    }

    static bool resizeFrame(const cv::Mat& src, cv::Mat &dst, int w, int h)
    {
        if (src.empty())
            return false;

        if (src.cols < w || src.rows < h || w <= 0 || h <= 0 || (src.cols == w && src.rows == h)) {
            src.copyTo(dst);
            return true;
        }

        float rate1 = (float)src.cols / w;
        float rate2 = (float)src.rows / h;
        float rate = std::min(rate1, rate2);
        cv::Size size;
        size.width = (int)(src.cols / rate);
        size.height = (int)(src.rows / rate);

        cv::Mat tmp;

        cv::resize(src, tmp, size);
        if (tmp.cols != w || tmp.rows != h) {
            int x = (tmp.cols - w) / 2;
            int y = (tmp.rows - h) / 2;
            dst = tmp(cv::Rect(x, y, w, h));
        }
        else {
            dst = tmp;
        }

        return true;
    }

    bool Capture::init()
    {
        bool res = false;
        if (!m_d)
            return false;

        m_d->abort = 0;
        setState(m_d, StateConnecting);

        while (!aborted(m_d) && !m_d->cap.isOpened()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(30));

            if (!m_d->filename.empty()) {
                m_d->cap.open(m_d->filename);
            }
            else if (m_d->devId >= 0) {
#if 0 || defined(_WIN32)
                m_d->cap.open(m_d->devId);
#else
                char camName[64];
                sprintf(camName, "/dev/video%d", m_d->devId);
                //SDL_Log("camera device: %s", camName);
                m_d->cap.open(camName);
#endif
            }
        }

        if (!m_d->cap.isOpened()) {
            //SDL_Log("Capture::open error!!!");
            setState(m_d, StateConnectionFailed);
            return false;
        }

        if (!m_d->filename.empty()) {
            m_d->frameCond = 0;
            m_d->fps = (float)m_d->cap.get(CV_CAP_PROP_FPS);
        }
        else if (m_d->capSize.width>0 && m_d->capSize.height>0) {
            m_d->cap.set(CV_CAP_PROP_FRAME_WIDTH, m_d->capSize.width);
            m_d->cap.set(CV_CAP_PROP_FRAME_HEIGHT, m_d->capSize.height);
            //m_d->cap.set(CV_CAP_PROP_FPS, 25);
        }

        return true;
    }

    bool Capture::open(int devId)
    {
        m_d->devId = devId;
        m_d->filename.clear();
        return open();
    }

    bool Capture::open(const std::string &filename)
    {
        m_d->devId = -1;
        m_d->filename = filename;
        return open();
    }

    bool Capture::open()
    {
        if (!m_d)
            return false;
        if (m_d->thread.joinable())
            return false;

        int ret = -1;

        m_d->abort = 0;
        m_d->state = 0;

        m_d->thread = std::thread(&Capture::threadFunc, this);
        return true;
    }

    void Capture::close()
    {
        if (!m_d)
            return;

        m_d->mutex.lock();
        m_d->abort = 1;
        m_d->mutex.unlock();

        if (m_d->frameCond > 0)
            m_d->frameCond = 0;

        if (m_d->thread.joinable())
            m_d->thread.join();

        m_d->cap.release();
    }

    bool Capture::isOpened() {
        if (!m_d)
            return false;
        return m_d->cap.isOpened();
    }

    bool Capture::isPlaying() {
        if (!m_d)
            return false;
        std::lock_guard<std::mutex> lock(m_d->mutex);
        return m_d->state == StatePlaying;
    }

    int Capture::state() {
        if (!m_d)
            return StateError;
        std::lock_guard<std::mutex> lock(m_d->mutex);
        return m_d->state;
    }

    inline static bool isVideo(CapPrivate *data) { return data->filename.empty() ? false : true; }

    int Capture::getFrame(Frame &frame, bool ignoreTime)
    {
        if (state() >= StateStoppedByUser)
            return -1;
        if (!isPlaying())
            return 0;

        std::lock_guard<std::mutex> lock(m_d->frameMutex);
        int ret = 0;
        if (ignoreTime || frame.time < 0 || frame.time < m_d->frameTime) {
            if (!m_d->frame.empty()) {
                if (m_d->cropSize.width>0 && m_d->cropSize.height>0)
                    resizeFrame(m_d->frame, frame.image, m_d->cropSize.width, m_d->cropSize.height);
                else
                    m_d->frame.copyTo(frame.image);
                ret++;
            }
            frame.number = m_d->frameNum;
            if (isVideo(m_d) && m_d->fps>1.0)
                frame.time = (int64)round(frame.number / m_d->fps * 1000.0);
            else
                frame.time = m_d->frameTime;
        }

        if (m_d->frameCond > 0)
            m_d->frameCond = 0;
        return ret;
    }

    bool Capture::appendFilter(CapFilter *filter)
    {
        if (filter == NULL)
            return false;

        std::lock_guard<std::mutex> lock(m_d->mutex);
        std::list<CapFilter*>::iterator it;

        for (it = m_d->filters.begin(); it != m_d->filters.end(); it++) {
            if (filter == *it)
                break;
        }
        if (it != m_d->filters.end())
            return false;

        m_d->filters.push_back(filter);
        return true;
    }

    bool Capture::removeFilter(CapFilter *filter)
    {
        if (filter == NULL)
            return false;

        std::lock_guard<std::mutex> lock(m_d->mutex);
        std::list<CapFilter*>::iterator it;

        for (it = m_d->filters.begin(); it != m_d->filters.end(); it++) {
            if (filter == *it)
                break;
        }
        if (it == m_d->filters.end())
            return false;

        m_d->filters.erase(it);
        return true;
    }

    static void waitCond(int &cond)
    {
        while (cond > 0)  {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    void Capture::threadFunc()
    {
        if (!init())
            return;

        //SDL_Log("Capture::run start");

        setState(m_d, StatePlaying);
        m_d->frameNum = -1;

        cv::Mat frame;
        PStdTimer timer;
        timer.tic();

        while (!aborted(m_d) && m_d->cap.grab()) {
            if (!m_d->cap.retrieve(frame)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                continue;
            }

            timer.toc();
#if 0
            cv::imshow("Capture", frame);
            cv::waitKey(1);
#endif
            m_d->frameMutex.lock();
            frame.copyTo(m_d->frame);
            m_d->frameNum++;
            m_d->frameTime = timer.elapsed();

            send2filters(m_d->frame, m_d->frameNum, m_d->frameTime);
            if (m_d->frameCond >= 0)
                m_d->frameCond = 1;
            m_d->frameMutex.unlock();
            waitCond(m_d->frameCond);
        }

        m_d->cap.release();
        setState(m_d, StateEnd);
        //SDL_Log("Capture::run end");

        return;
    }

    int Capture::send2filters(cv::Mat &frame, int64 frameNum, int64 frameTime)
    {
        std::lock_guard<std::mutex> lock(m_d->mutex);
        std::list<CapFilter*>::iterator it;
        int count = 0;
        for (it = m_d->filters.begin(); it != m_d->filters.end(); it++) {
            CapFilter *filter = *it;
            if (filter) {
                count++;
                filter->send(frame, frameNum, frameTime);
            }
        }
        return count;
    }
}

