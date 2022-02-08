// ce_infer_thread_p.h
//
#ifndef CAR_EYE_INFER_THREAD_P_H
#define CAR_EYE_INFER_THREAD_P_H

#include "SDL_helper.h"

namespace ce {

    class InferThreadPrivate
    {
    public:
        InferThreadPrivate() {
            mutex = NULL;
            infer_tid = NULL;
            abort = 0;
            state = 0;
            cap = NULL;
            infer = NULL;
            resMutex = NULL;
        }
        SDL_mutex *mutex;
        SDL_Thread *infer_tid;
        int abort;
        int state;

        BinocsCap *cap;
        Infer *infer;
        SDL_mutex *resMutex;
        InferRes res;
    };
};

#endif //CAR_EYE_INFER_THREAD_P_H
