#include <math.h>
#include "ce_binocular_tracker.h"
#include "ce_math.hpp"

namespace ce {


	BinoTracker::BinoTracker(double threshold, int queue)
	{
        m_serialNum = 0;
		m_queue = queue;
		m_threshold = threshold;
	}

	BinoTracker::~BinoTracker()
	{
		m_prevs.clear();
	}

#if 0
    void BinoTracker::track(std::vector<BinoObj> &objs)
    {
        if (objs.size() == 0)
            return;

        std::map<int, BinoObj> id2obj, id2invobj;
        for (int t = 0; t < m_prevs.size(); ++t) {
            for (int j = 0; j < m_prevs[t].size(); ++j) {
                // id2obj[m_prevs[t][j].id] = m_prevs[t][j];
                id2obj.insert(std::make_pair(m_prevs[t][j].id, m_prevs[t][j]));
            }
            for (int j = 0; j < m_prevs[m_prevs.size() - 1 - t].size(); ++j) {
                id2invobj.insert(std::make_pair(m_prevs[m_prevs.size() - 1 - t][j].id, m_prevs[m_prevs.size() - 1 - t][j]));
            }
        }
        // objs compare to m_prevs
        for (int i = 0; i < objs.size(); ++i) {
            float tmp = FLT_MAX;
            std::map<int, BinoObj>::iterator iter;
            float dis = 0;
            for (iter = id2obj.begin(); iter != id2obj.end(); ++iter) {
                dis = ce::L2distance(objs[i].features, iter->second.features);
                if (dis <= m_threshold && tmp > dis) {
                    tmp = dis;
                    objs[i].id = iter->first;
                    objs[i].velocity = (objs[i].distance - id2invobj[objs[i].id].distance)* 1000.0 / (objs[i].frameTime - id2invobj[objs[i].id].frameTime);
                    objs[i].velocity = objs[i].velocity * 3.6;
                }
            }
            if (objs[i].id <= 0) {
                m_serialNum ++;
                objs[i].id = m_serialNum;
            }
            else {
                if (id2obj.count(objs[i].id))
                    id2obj.erase(objs[i].id);
            }
        }

        // objs push to m_prevs
        if (m_prevs.size() >= m_queue) {
            m_prevs.erase(m_prevs.begin());
        }
        m_prevs.push_back(objs);
    }
#else
    static int getPrevObjs(std::vector<BinoObjList> &prevs, id_t id, std::vector<BinoObj*> &objs)
    {
        for (int i = 0; i < (int)prevs.size(); i++) {
            BinoObjList &prevObjs = prevs[i];
            for (int j = 0; j < (int)prevObjs.size(); j++) {
                if (prevObjs[j].id == id) {
                    objs.push_back(&prevObjs[j]);
                    break;
                }
            }
        }
        return (int)objs.size();
    }

    static double calcDistance(std::vector<BinoObj*> prevObjs, BinoObj &obj, int avgCount = 3)
    {
        double dist = obj.distance;
        double count = 0;
        for (int i = (int)prevObjs.size() - 1; i >= 0 && count < avgCount; i--) {
            dist += prevObjs[i]->distance;
            count++;
        }
        if (dist > 0.001 && count>0)
            dist /= (count + 1);
        dist = round(dist * 10) / 10;
        return dist;
    }

    void BinoTracker::track(BinoObjList &objs)
    {
        if (objs.size() == 0)
            return;

        findObjIds(objs);

        for (int i = 0; i < (int)objs.size(); i++) {
            std::vector<BinoObj*> prevObjs;
            BinoObj &obj = objs[i];
            getPrevObjs(m_prevs, obj.id, prevObjs);

            //calculate average distance
            obj.distance = calcDistance(prevObjs, obj);
            if (prevObjs.size()>0) {
                BinoObj *prevObj = prevObjs[0];
                if (abs(obj.frameTime - prevObj->frameTime) > 0) {
                    obj.velocity = (prevObj->distance - obj.distance) / (obj.frameTime - prevObj->frameTime);
                    obj.velocity = obj.velocity*3600;
                }
            }
        }

        // objs push to m_prevs
        if (m_prevs.size() >= m_queue) {
            m_prevs.erase(m_prevs.begin());
        }
        m_prevs.push_back(objs);
    }
#endif
    void BinoTracker::findObjIds(BinoObjList &objs)
    {
        std::map<id_t, BinoObj*> map;
        for (int i = 0; i < (int)objs.size(); i++) {
            id_t id = find(objs[i], map);
            if (id == 0) {
                m_serialNum++;
                objs[i].id = m_serialNum;
            }
            else {
                objs[i].id = id;
                map[id] = &objs[i];
            }
        }
    }
    id_t BinoTracker::find(BinoObj &obj, std::map<id_t, BinoObj*> &usedMap)
    {
        cv::Rect2f objRect(obj.left.x, obj.left.y, obj.left.width, obj.left.height);
        cv::Point2f cpt(obj.left.x + obj.left.width / 2, obj.left.y + obj.left.height / 2);
        id_t id = 0;
        float mindist = 0;
        float dist;

        for (int i = (int)m_prevs.size() - 1; i >= 0; i--) {
            BinoObjList &prevList = m_prevs[i];
            for (int j = 0; j < (int)prevList.size(); j++) {
                BinoObj &prev = prevList[j];
                if (usedMap.count(prev.id) > 0)
                    continue;
                cv::Rect2f prevRect(prev.left.x, prev.left.y, prev.left.width, prev.left.height);
                if (!prevRect.contains(cpt))
                    continue;

                dist = ce::L2distance(obj.left.features, prev.left.features);
                if (dist <= m_threshold) {
                    if (id == 0 || mindist > dist) {
                        id = prev.id;
                        mindist = dist;
                    }
                }
            }
        }
        return id;
    }
}