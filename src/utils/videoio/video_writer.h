// video_writer.h
//
#ifndef __VIDEO_WRITER_H__
#define __VIDEO_WRITER_H__

#include <string>
#include <vector>

#include "video_cap.h"

namespace videoio {
    class WriterPrivate;

    class LIB_VIDEOIO_API Writer
    {
    public:
        enum state {
            StateNone = 0,
            StateRunning,
            StateStoppedByUser,
            StateEnd,

            //error
            StateError,
            StateConnectionFailed,
            StateStoppedWithError,
        };

        Writer(Capture *cap, const char *dirname, const char *prefix = NULL, int fps = 25, int step = 60);
        ~Writer();

        void setCropRect(int x, int y, int width, int height);
        bool start();
        void stop();
        bool isRunning();
        int receive(cv::Mat &frame, int64 frameNum, int64 frameTime);

    protected:
        void threadFunc();
        bool init();
        void release();
        void checkTime();
        bool createWriter();
        void releaseWriter();
        bool writeFrame(Frame &wf);

        WriterPrivate *m_d;

    public:
        int run();
    };
};

#endif //__VIDEO_WRITER_H__
