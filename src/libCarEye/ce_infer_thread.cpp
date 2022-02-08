#include "ce_infer_thread.h"
#include "ce_infer_thread_p.h"
#include "TimerCV.h"

namespace ce {

    InferThread::InferThread(BinocsCap *cap, Infer *infer)
	{
        m_d = new InferThreadPrivate();
        m_d->mutex = SDL_CreateMutex();
        m_d->cap = cap;
        m_d->infer = infer;
        m_d->resMutex = SDL_CreateMutex();
    }

	InferThread::~InferThread()
	{
        if (m_d) {
            stop();
            if (m_d->resMutex)
                SDL_DestroyMutex(m_d->resMutex);
            if (m_d->mutex)
                SDL_DestroyMutex(m_d->mutex);
            delete m_d;
            m_d = NULL;
        }
	}

    bool InferThread::init()
    {
        if (!m_d->infer->initAll())
            return false;

        return true;
    }

    void InferThread::release()
    {
    }

    static int infer_thread(void *arg)
    {
        InferThread *t = (InferThread*)arg;
        if (t)
            return t->run();
        return 0;
    }

    bool InferThread::start()
    {
        int ret = -1;

        m_d->abort = 0;

        if (m_d && !m_d->infer_tid) {
            m_d->infer_tid = SDL_CreateThread(infer_thread, "infer_thread", this);
            if (m_d->infer_tid)
                ret = 0;
        }
        return (ret == 0) ? true : false;
    }

    void InferThread::stop()
    {
        if (state()>0) {
            SDL_LockMutex(m_d->mutex); 
            m_d->abort = 1;
            SDL_UnlockMutex(m_d->mutex);

            while (state()>0)
                SDL_Delay(10);
        }
    }

    int InferThread::state() {
        if (!m_d)
            return -1;
        SDL_MutexLocker locker(m_d->mutex);
        return m_d->state;
    }


    static int aborted(InferThreadPrivate *p) {
        SDL_MutexLocker locker(p->mutex);
        return p->abort;
    }

    static void setState(InferThreadPrivate *p, int state) {
        SDL_LockMutex(p->mutex);
        p->state = state;
        SDL_UnlockMutex(p->mutex);
    }

    int InferThread::getInferRes(InferRes &res)
    {
        SDL_LockMutex(m_d->resMutex);
        res.fps = m_d->res.fps;
        res.frames = m_d->res.frames;
        m_d->res.frame.copyTo(res.frame);
        if (res.frames == 1)
            res.detInfos = m_d->res.detInfos;
        else if (res.frames == 2)
            res.inferInfos = m_d->res.inferInfos;
        SDL_UnlockMutex(m_d->resMutex);

        return res.frames;
    }

    int InferThread::run()
    {
        if (!init())
            return -1;

        setState(m_d, 1);

        int ret = 0;
        ce::BinocsFrame frame;
        TimerCV timer;
        double fps;
        std::vector<ce::DetInfo> dets;
        std::vector<ce::InferInfo> infers;

        while (!aborted(m_d)) {
            timer.Tic();
            ret = m_d->cap->getFrame(frame);
            if (ret == 1) {
                m_d->infer->run(frame.frame[0], &dets);
                timer.Toc();
                fps = 1000.0 / (double)timer.Elapsed();
            }
            else if (ret == 2) {
                infers = m_d->infer->run(frame.frame[0], frame.frame[1], frame.frameTime);
                timer.Toc();
                fps = 1000.0 / (double)timer.Elapsed();
            }
            else {
                SDL_Delay(10);
                continue;
            }
            //SDL_Log("frames=%d", ret);

            //copy infer result
            SDL_LockMutex(m_d->resMutex);
            m_d->res.fps = fps;
            m_d->res.frames = ret;
            if (ret == 1)
                m_d->res.detInfos = dets;
            else if (ret==2)
                m_d->res.inferInfos = infers;
            frame.copyTo(m_d->res.frame);
            SDL_UnlockMutex(m_d->resMutex);
        }

        release();
        setState(m_d, -1);

        return 0;
    }
}
