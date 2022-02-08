#include <iostream>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include "ce_detector.h"

namespace ce {

	static char *s_labelNames[] = { "bicycle", "bus", "car", "motorbike", "person", /*"train",*/
		"motorcycle", "truck", NULL};

	Detector::Detector(const char *dirName, const char *prototxt, const char *weights, const char *label)
	{
		pic::dnn::ModelParams param = { prototxt, weights, "", label };
		m_net = pic::dnn::Detector::create(dirName, param);

		m_threshold = DNN_THRESHOLD;
        m_minSize = 48;
	}

	Detector::~Detector()
	{
		if (m_net)
			delete m_net;
		m_net = NULL;
	}

    static int checkRange(int width, int height, pic::dnn::DetectedObjList &objs)
    {
        cv::Rect2f roiRect((float)width / 3, (float)height / 2, (float)width / 3, (float)height / 2);

        for (int i = (int)objs.size() - 1; i >= 0; i--) {
            pic::dnn::DetectedObj &obj = objs[i];
            cv::Rect2f objRect(obj.x, obj.y, obj.width, obj.height);
            cv::Rect2f crossRect = roiRect & objRect;
            if (crossRect.width < 1 && crossRect.height < 1) {
                objs.erase(objs.begin() + i);
            }
        }
        return (int)objs.size();
    }

	static int checkIntersection(pic::dnn::DetectedObjList &objs)
    {
        cv::Rect_<float> rc1, rc2;
        cv::Rect_<float> rcc; //rect for common

        cv::Point2f pt1, pt2;
        for (int i = 0; i < (int)objs.size()-1; i++) {
            pic::dnn::DetectedObj &obj1 = objs[i];
            rc1 = cv::Rect_<float>(obj1.x, obj1.y, obj1.width, obj1.height);
            pt1.x = obj1.x + obj1.width / 2;
            pt1.y = obj1.y + obj1.height / 2;
            for (int j = i + 1; j < (int)objs.size(); j++) {
                pic::dnn::DetectedObj &obj2 = objs[j];
                rc2 = cv::Rect_<float>(obj2.x, obj2.y, obj2.width, obj2.height);
                pt2.x = obj2.x + obj2.width / 2;
                pt2.y = obj2.y + obj2.height / 2;

                rcc = rc1 & rc2;
                int rmIdx = -1;
				if (rcc.contains(pt2) || rcc.contains(pt1)) {
					if (obj1.score > obj2.score)
						rmIdx = j;
					else
						rmIdx = i;
					objs.erase(objs.begin() + rmIdx);
					j--;
					if (rmIdx == i) {
						i--;
						break;
					}
				}
            }
        }
        return (int)objs.size();
    }

    static int adjustObjRect(pic::dnn::DetectedObj &obj1, pic::dnn::DetectedObj &obj2)
    {
        int res = 0;
        cv::Rect2f rc1(obj1.x, obj1.y, obj1.width, obj1.height);
        cv::Rect2f rc2(obj2.x, obj2.y, obj2.width, obj2.height);
        cv::Rect2f rcc = rc1 & rc2;
        if (rcc.width < 0.01 || rcc.height < 0.01)
            return 0;

        cv::Point2f pt1(rc1.x + rc1.width / 2, rc1.y + rc1.height / 2);
        cv::Point2f pt2(rc2.x + rc2.width / 2, rc2.y + rc2.height / 2);

        if (rc1.contains(pt2) || rc2.contains(pt1)) {
            if (rc1.contains(pt2) && rc2.contains(pt1)) {
                if (obj1.score>obj2.score)
                    return 1;
                return -1;
            }
            return rc1.contains(pt2) ? 1 : -1;
        }

        float dw = rcc.width / 2;
        obj1.width -= dw;
        obj2.width -= dw;
        if (obj1.x < obj2.x)
            obj2.x += dw;
        else
            obj1.x += dw;
        return 0;
    }

    static void adjustDetectedObjects(pic::dnn::DetectedObjList &objs)
    {
        if (objs.size() < 2)
            return;

        for (int i = 0; i < (int)objs.size() - 1; i++) {
            pic::dnn::DetectedObj &obj1 = objs[i];
            for (int j = i + 1; j < (int)objs.size(); j++) {
                pic::dnn::DetectedObj &obj2 = objs[j];
                int res = adjustObjRect(obj1, obj2);
                if (res > 0) {
                    objs.erase(objs.begin() + j);
                    j--;
                    continue;
                }
                else if (res<0) {
                    objs.erase(objs.begin() + i);
                    i--;
                    break;
                }
            }
        }
    }

    int Detector::detect(cv::Mat &img, pic::dnn::DetectedObjList &objs)
    {
		if (!m_net)
			return 0;
        m_net->detect(img, m_threshold, objs);
#if 1
        //check min size
        if (m_minSize>0) {
            for (int i=0;i<(int)objs.size(); i++) {
                if (objs[i].height<m_minSize && objs[i].width<m_minSize) {
                    objs.erase(objs.begin()+i);
                    i--;
                }
            }
        }
#endif
		checkLabels(objs);
        checkRange(img.cols, img.rows, objs);
		checkIntersection(objs);
        adjustDetectedObjects(objs);
        return (int)objs.size();
    }

    std::string  Detector::labelName(int label)
    {
		if (!m_net)
			return "";
        return m_net->label(label);
    }


	int Detector::checkLabels(pic::dnn::DetectedObjList &objs)
	{
		std::string objname;
		for (int i = 0; i < (int)objs.size(); i++) {
			if (objs[i].label < 0)
				continue;

			objname = m_net->label(objs[i].label);

			int j = 0;
			for (j = 0; s_labelNames[j] != NULL; j++) {
				if (objname.compare(s_labelNames[j]) == 0)
					break;
			}
			if (s_labelNames[j] == NULL) {
				objs.erase(objs.begin() + i);
				i--;
			}
		}
		return (int)objs.size();
	}
}
