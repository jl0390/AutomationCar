// ce_binocular_tracker.h
//

#ifndef __CAR_EYE_BINOCULAR_TRACKER_H__
#define __CAR_EYE_BINOCULAR_TRACKER_H__

#include "ce_types.h"
#include "ce_object_types.hpp"
#include "ce_descriptor_matcher.h"

namespace ce {
	class BinoTracker {
	public:
		BinoTracker(double threshold = 50.0, int queue = 10);
		~BinoTracker();

        void track(BinoObjList &objs);

	protected:
        void findObjIds(BinoObjList &objs);
        id_t find(BinoObj &obj, std::map<id_t, BinoObj*> &usedMap);

		double m_threshold;
		int64 m_serialNum;
		int m_queue;
        std::vector<BinoObjList> m_prevs;
	};
}


#endif //__CAR_EYE_BINOCULAR_TRACKER_H__