#include <time.h>
#include <list>
#include <mutex>
#include <thread>

#include "video_writer.h"

#ifdef WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#define _mkdir(a) { umask(0); mkdir((a), 0755); }
#endif

#define WRITE_TIME_STEP 60  //min
#define WRITE_FPS       25  //frames per second
#define VIDEO_FILE_EXT	".avi"

#define VIDEO_FOURCC	CV_FOURCC('X', 'V', 'I', 'D')
#define FRAME_MAX_SIZE  3

namespace videoio {
    class WriterFilter : public CapFilter
    {
    public:
        WriterFilter(Writer *writer) {
            m_writer = writer;
        }
        virtual ~WriterFilter() {}

        virtual int send(cv::Mat &frame, int64 frameNum, int64 frameTime) {
            return m_writer->receive(frame, frameNum, frameTime);
        }
    private:
        Writer* m_writer;
    };

    class WriterPrivate
    {
    public:
        WriterPrivate() {
            cap = NULL;
            fps = 25;

            abort = 0;
            state = 0;

            writer = NULL;
            frameCount = 0;
            fileStartTime = 0;
            fileEndTime = 0;
            firstFrameTime = 0;
        }
        //config
        Capture *cap;
        std::string dirname;
        std::string prefix;
        int fps;
        int step;   //ms
        cv::Size frameSize;
        cv::Rect cropRect;

        std::mutex mutex;
        std::thread thread;
        int abort;
        int state;

        WriterFilter *filter;

        std::mutex frameMutex;
        std::list<Frame> frames;

        //writing information
        cv::VideoWriter *writer;
        time_t fileStartTime;   //sec
        time_t fileEndTime;   //sec
        int64 frameCount;
        int64 firstFrameTime;   //ms
        Frame lastFrame;
    };

    Writer::Writer(Capture *cap, const char *dirname, const char *prefix, int fps, int step)
    {
        m_d = new WriterPrivate();
        m_d->cap = cap;
        m_d->dirname = dirname;
        m_d->prefix = prefix ? prefix : "";
        m_d->fps = fps > 0 ? fps : WRITE_FPS;
        m_d->step = step > 0 ? step : WRITE_TIME_STEP;  //second

        m_d->filter = new WriterFilter(this);
        cap->appendFilter(m_d->filter);
    }

    Writer::~Writer()
    {
        if (isRunning())
            stop();
        if (m_d) {
            if (m_d->cap && m_d->filter)
                m_d->cap->removeFilter(m_d->filter);
            if (m_d->filter)

                delete m_d->filter;
            delete m_d;
            m_d = NULL;
        }
    }

    static void setState(WriterPrivate *p, int state) {
        std::lock_guard<std::mutex> lock(p->mutex);
        p->state = state;
    }

    bool Writer::init()
    {
        _mkdir(m_d->dirname.c_str());
        return true;
    }

    void Writer::release()
    {
    }

    void Writer::setCropRect(int x, int y, int width, int height)
    {
        m_d->cropRect = cv::Rect(x, y, width, height);
    }

    bool Writer::start()
    {
        if (!m_d)
            return false;
        if (m_d->thread.joinable())
            return false;

        int ret = -1;

        m_d->abort = 0;
        m_d->state = 0;

        m_d->thread = std::thread(&Writer::threadFunc, this);
        return true;
    }

    void Writer::stop()
    {
        if (!m_d)
            return;

        m_d->mutex.lock();
        m_d->abort = 1;
        m_d->mutex.unlock();

        if (m_d->thread.joinable())
            m_d->thread.join();
    }

    bool Writer::isRunning() {
        if (!m_d)
            return false;
        std::lock_guard<std::mutex> lock(m_d->mutex);
        return m_d->state == Writer::StateRunning ? true : false;
    }

    static int aborted(WriterPrivate *p) {
        std::lock_guard<std::mutex> locker(p->mutex);
        return p->abort;
    }

    static bool isBinocsFrame(cv::Size &size)
    {
        bool res = false;
        float rate = (float)size.width * 3 / size.height;
        if (rate - 8 > -0.00001)
            res = true;
        return res;
    }

    void Writer::threadFunc()
    {
        if (!init())
            return;

        setState(m_d, Writer::StateRunning);
        Frame wf;

        while (!aborted(m_d)) {
            bool empty = true;
            m_d->frameMutex.lock();
            if (m_d->frames.size() > 0) {
                wf = m_d->frames.front();
                m_d->frames.pop_front();
                empty = false;
            }
            m_d->frameMutex.unlock();
            if (empty) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                continue;
            }

            m_d->frameSize = wf.image.size();
            checkTime();

            if (!m_d->writer){
                setState(m_d, Writer::StateStoppedWithError);
                break;
            }

            writeFrame(wf);
        }

        releaseWriter();
        release();
        if (state() < Writer::StateEnd)
            setState(m_d, Writer::StateEnd);

        return;
    }

    int Writer::receive(cv::Mat &frame, int64 frameNum, int64 frameTime)
    {
        //printf("receive : %ld, %ld\n", (long)frameNum, (long)frameTime);
        if (!isRunning())
            return -1;

        std::lock_guard<std::mutex> locker(m_d->frameMutex);
        if (m_d->frames.size() >= FRAME_MAX_SIZE)
            m_d->frames.pop_front();

        Frame wf;
        wf.image = frame;
        wf.number = frameNum;
        wf.time = frameTime;
        m_d->frames.push_back(wf);

        return 0;
    }

    void Writer::checkTime()
    {
        if (!m_d->writer || time(NULL) >= m_d->fileEndTime) {
            if (m_d->writer)
                releaseWriter();
            createWriter();
        }
    }

    static std::string time2str(time_t time, int onlyTime = 0)
    {
        std::tm tm = *localtime(&time);
        char str[128];

        tm.tm_year %= 100;
        tm.tm_mon++;

        if (onlyTime)
            sprintf(str, "%02d%02d", tm.tm_hour, tm.tm_min);
        else
            sprintf(str, "%02d%02d%02d_%02d%02d", tm.tm_year, tm.tm_mon, tm.tm_mday, tm.tm_hour, tm.tm_min);
        return str;
    }

    bool Writer::createWriter()
    {
        int64 startTime = m_d->fileEndTime == 0 ? time(NULL) : m_d->fileEndTime;
        int64 endTime = startTime + m_d->step * 60;
        endTime /= (m_d->step * 60);
        endTime *= (m_d->step * 60);

        std::string stime = time2str(startTime);
        std::string etime = time2str(endTime, 1);

        std::string fname = m_d->dirname + "/";
        fname += m_d->prefix + stime + "_";
        fname += etime;
        fname += VIDEO_FILE_EXT;

        m_d->writer = new cv::VideoWriter();
        cv::Size frameSize = m_d->cropRect.size();
        if (frameSize.width <= 0 || frameSize.height <= 0)
            frameSize = m_d->frameSize;
        if (m_d->writer->open(fname, VIDEO_FOURCC, m_d->fps, frameSize)) {
            m_d->frameCount = 0;
            m_d->fileStartTime = startTime;
            m_d->fileEndTime = endTime;
            return true;
        }

        delete m_d->writer;
        m_d->writer = NULL;
        return false;
    }

    void Writer::releaseWriter()
    {
        if (m_d->writer) {
            m_d->writer->release();
            delete m_d->writer;
            m_d->writer = NULL;
        }
    }

    static void cropImage(cv::Mat &src, cv::Mat &dst, const cv::Rect &rect)
    {
        if (rect.width > 0 && rect.height > 0)
            dst = src(rect);
        else
            dst = src;
    }

    bool Writer::writeFrame(Frame &wf)
    {
        if (!m_d->writer->isOpened())
            return false;

        cv::Mat img;
        if (m_d->frameCount == 0) {
            cropImage(wf.image, img, m_d->cropRect);
            m_d->writer->write(img);
            m_d->frameCount++;
            m_d->firstFrameTime = wf.time;
        }
        else {
            int64 msec = wf.time - m_d->firstFrameTime;
            int64 frameNum = (wf.time - m_d->firstFrameTime)*m_d->fps / 1000;

            //printf("time=%ld ms, frame_num=%ld, frame_count=%ld\n", (long)msec, (long)frameNum, m_d->frameCount);
            if (frameNum >= m_d->frameCount) {
                while (m_d->frameCount < frameNum) {
                    cropImage(m_d->lastFrame.image, img, m_d->cropRect);
                    m_d->writer->write(img);
                    m_d->frameCount++;
                    //printf("repeat %ld\n", (long)m_d->frameCount);
                }
                cropImage(wf.image, img, m_d->cropRect);
                m_d->writer->write(img);
                m_d->frameCount++;
                //printf("write frame_num=%ld, frame_count=%ld\n", (long)frameNum, m_d->frameCount);
            }
        }
        m_d->lastFrame = wf;
        return true;
    }
}
