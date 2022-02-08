// videoio/video_cap.h
//
#ifndef __VIDEO_CAP_H__
#define __VIDEO_CAP_H__

#include <string>
#include <vector>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#if CV_MAJOR_VERSION>=3
#include <opencv2/videoio.hpp>
#endif
#include "opencv_ext.hpp"

#ifndef VIDEOIO_SHARED
#define LIB_VIDEOIO_API
#else
#ifdef QT_CORE_LIB
#include <QtCore/qglobal.h>

#ifdef LIB_EXPORTS
# define LIB_VIDEOIO_API Q_DECL_EXPORT
#else
# define LIB_VIDEOIO_API Q_DECL_IMPORT
#endif
#else
#ifdef LIB_EXPORTS
#define LIB_VIDEOIO_API __declspec(dllexport)
#else
#define LIB_VIDEOIO_API __declspec(dllimport)
#endif
#endif
#endif //VIDEO_CAP_STATIC

namespace videoio {

    enum state {
        StateNone = 0,
        StateConnecting,
        StatePlaying,
        StateStoppedByUser,
        StateEnd,

        //error
        StateError,
        StateConnectionFailed,
        StateStoppedWithError,
    };

    class CapPrivate;

    class Frame {
    public:
        Frame() {
            number = -1;
            time = 0;
        }
        cv::Mat image;
        int64 number;
        int64 time; //ms
    };

    class CapFilter {
    public:
        CapFilter() {}
        virtual ~CapFilter() {}
        virtual int send(cv::Mat &frame, int64 frameNum, int64 frameTime) = 0;
    };

    class LIB_VIDEOIO_API Capture
    {
    public:
        Capture(int devId = 0);
        Capture(const char* video_file);
        ~Capture();

        void setCaptureSize(int w, int h);
        void setCropSize(int w, int h);

        bool open();
        bool open(int devId);
        bool open(const std::string &filename);
        void close();
        bool isOpened();
        bool isPlaying();
        int state();
        int getFrame(Frame &frame, bool ignoreTime = false);
        int read(Frame &frame, bool ignoreTime = false) { return getFrame(frame, ignoreTime); }

        bool appendFilter(CapFilter *filter);
        bool removeFilter(CapFilter *filter);

    protected:
        void threadFunc();
        bool init();
        int send2filters(cv::Mat &frame, int64 frameNum, int64 frameTime);

        CapPrivate *m_d;

    public:
        int run();  //thread
    };
};

#endif //__VIDEO_CAP_H__
