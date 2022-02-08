// ce_infer_thread.h
//
#ifndef CAR_EYE_INFER_THREAD_H
#define CAR_EYE_INFER_THREAD_H

#include "ce_types.h"
#include "ce_binocs_cap.h"
#include "ce_infer.h"

namespace ce {

    class InferRes {
    public:
        ce::BinocsFrame frame;
        std::vector<ce::DetInfo> detInfos;
        std::vector<ce::InferInfo> inferInfos;
        double fps;
        int frames;
    };

    class InferThreadPrivate;

    class LIBCE_API InferThread
	{
	public:
        InferThread(BinocsCap *cap, Infer *infer);
        ~InferThread();

        bool start();
        void stop();
        int state();

        int getInferRes(InferRes &res); //return frames

    protected:
        bool init();
        void release();

        InferThreadPrivate *m_d;

    public:
        int run();  //thread
	};
};

#endif //CAR_EYE_INFER_THREAD_H
