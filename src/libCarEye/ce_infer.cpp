#include "ce_infer.h"
#include "ce_detector.h"
#if CE_USE_IMAGENET_FEATURE
#include "ce_classifier.h"
#endif
#if CE_USE_RANGE_FINDER
#include "ce_range_finder.h"
#endif
#include "ce_binocular_tracker.h"

#include <iostream>
#include "filesystem.h"
#include "pstdthread.h"

namespace ce
{
    class InferPrivate {
    public:
        CE_MODEL_INFO detectParam;
        Detector *detector;
        BinoTracker *tracker;
#if CE_USE_IMAGENET_FEATURE
        CE_MODEL_INFO classParam;
        Classifier *classifier;
#endif
#if CE_USE_RANGE_FINDER
        CE_CAMERA_INFO finderParam;
        RangeFinder *finder;
#endif
        BinocsCamera *camera;
        std::string cameraParam;
        cv::Ptr<cv::FastFeatureDetector> featureDetector;
    };

	Infer::Infer()
	{
        m_d = new InferPrivate();

		m_d->detector = NULL;
		m_d->tracker = NULL;
#if CE_USE_IMAGENET_FEATURE
        m_d->classifier = NULL;
#endif
#if CE_USE_RANGE_FINDER
        m_d->finder = NULL;
#endif
        m_d->camera = NULL;
    }

	Infer::~Infer()
	{
        if (!m_d)
            return;

		if (m_d->detector)
			delete m_d->detector;
#if CE_USE_IMAGENET_FEATURE
        if (m_d->classifier)
			delete m_d->classifier;
#endif
#if CE_USE_RANGE_FINDER
        if (m_d->finder)
			delete m_d->finder;
#endif
		if (m_d->tracker)
			delete m_d->tracker;

#if 0
        if (m_d->featureDetector)
            m_d->featureDetector.release();
#endif
        delete m_d;
        m_d = NULL;
	}

    void Infer::setDetectModel(const CE_MODEL_INFO &model)
    {
        m_d->detectParam = model;
    }

#if CE_USE_IMAGENET_FEATURE
    void Infer::setClassifyModel(const CE_MODEL_INFO &model)
    {
        m_d->classParam = model;
    }
#endif
#if CE_USE_RANGE_FINDER
    void Infer::setRangeFinder(const CE_CAMERA_INFO &camInfo)
    {
        m_d->finderParam = camInfo;
    }
#endif
    void Infer::setCamera(BinocsCamera *camera)
    {
        m_d->camera = camera;
    }

    void Infer::setCameraParam(const std::string &filename)
    {
        m_d->cameraParam = filename;
    }

    bool Infer::initAll()
    {
        if (!initDetectModel(m_d->detectParam.path, m_d->detectParam.protoFile, m_d->detectParam.modelFile, m_d->detectParam.labelFile))
            return false;
#if CE_USE_IMAGENET_FEATURE
        if (!initClassifyModel(m_d->classParam.path, m_d->classParam.protoFile, m_d->classParam.modelFile,
			m_d->classParam.meanFile, m_d->classParam.labelFile))
            return false;
#endif
#if CE_USE_RANGE_FINDER
        if (!initRangeFinder(m_d->finderParam.dist, m_d->finderParam.focus, m_d->finderParam.delta, m_d->classParam.threshold))
            return false;
#endif
        if (!initTracker())
            return false;

#if CE_USE_STEREO_CALIB
        if (!initStereoCalib())
            return false;
#endif
        m_d->featureDetector = cv::FastFeatureDetector::create();
        return true;
    }

	bool Infer::initDetectModel(const char *dirName, const char *prototxt, const char *weights, const char *labelMap)
	{
		if (m_d->detector)
			delete m_d->detector;

		m_d->detector = new ce::Detector(dirName, prototxt, weights, labelMap);
        if (!m_d->detector->net()) {
			std::cerr << "Coult not create ce::Detector" << std::endl;
			return false;
		}
		m_d->detector->setThreshold(m_d->detectParam.threshold);
		return true;
	}

#if CE_USE_IMAGENET_FEATURE
	bool Infer::initClassifyModel(const char *dirName, const char *prototxt, const char *weights, const char *mean, const char *labelmap)
	{
		if (m_d->classifier)
			delete m_d->classifier;

		m_d->classifier = new ce::Classifier(dirName, prototxt, weights, mean, labelmap);
		if (!m_d->classifier->net()) {
			std::cerr << "Coult not create ce::Classifier" << std::endl;
			return false;
		}

		return true;
	}
#endif
#if CE_USE_RANGE_FINDER
    bool Infer::initRangeFinder(double cam_dist, double cam_focus, double delta, double threshold)
	{
		if (m_d->finder)
			delete m_d->finder;

		m_d->finder = new ce::RangeFinder(cam_dist, cam_focus, delta, threshold);
		if (!m_d->finder) {
			std::cerr << "Coult not create ce::RangeFinder" << std::endl;
			return false;
		}

		return true;
	}
#endif
	bool Infer::initTracker()
	{
		if (m_d->tracker)
			delete m_d->tracker;

		m_d->tracker = new ce::BinoTracker();
		if (!m_d->tracker) {
			std::cerr << "Coult not create ce::BinoTracker" << std::endl;
			return false;
		}

		return true;
	}

    int Infer::run(cv::Mat &left_frame, cv::Mat &right_frame, int64 time, std::vector<InferInfo> &infers)
    {
        PStdTimer timer;
        timer.tic();
        // detect objects in left and right frames;
        pic::dnn::DetectedObjList leftobjs, rightobjs;
        m_d->detector->detect(left_frame, leftobjs);
        m_d->detector->detect(right_frame, rightobjs);
        timer.toc();
        int64 detectTime = timer.elapsed();

        timer.tic();
        // convert ssd:DetectedObjList to ce::MonoObjList and extract features.
        std::vector<ce::MonoObj> leftmonos, rightmonos;
        extractFeatures(left_frame, leftobjs, leftmonos);
        extractFeatures(right_frame, rightobjs, rightmonos);
        timer.toc();
        int64 extFeatTime = timer.elapsed();

        timer.tic();
#if !CE_USE_STEREO_CALIB
        std::vector<ce::BinoObj> binoObjs = m_d->finder->computeDistance(left_frame, right_frame, leftmonos, rightmonos, time);
#else //CE_USE_STEREO_CALIB
        std::vector<ce::BinoObj> binoObjs;
        m_d->finder->findObjs(left_frame, right_frame, leftmonos, rightmonos, binoObjs);
        for (int i = 0; i < (int)binoObjs.size(); i++) {
            BinoObj &obj = binoObjs[i];
            obj.frameTime = time;
            obj.distance = m_d->camera->calcDistance(obj.left.keypnts, obj.right.keypnts, obj.matches,
                left_frame.cols, left_frame.rows);
        }
#endif
#if CE_DEBUG
        m_d->camera->remap(left_frame, right_frame);
#endif
        m_d->tracker->track(binoObjs);
        timer.toc();
        int64 computeDistTime = timer.elapsed();

        infers.clear();
        createInferList(binoObjs, infers);

        //std::cout << "Times detect: " << detectTime << " ms, extract_feat: " << extFeatTime << " ms, compute_dist: " << computeDistTime << " ms" << std::endl;
        return (int)infers.size();
    }

    int Infer::run(cv::Mat &frame, std::vector<InferInfo> &infers)
    {
        std::vector<DNN_DetectedObj> objs;
        m_d->detector->detect(frame, objs);

        infers.clear();

        // convert std::vector<DNN_DetectedObj> to std::vector<ce::DetInfo>
        for (int i = 0; i < objs.size(); ++i) {
            DNN_DetectedObj obj = objs[i];
            ce::InferInfo infer;
            infer.left.x = obj.x;
            infer.left.y = obj.y;
            infer.left.width = obj.width;
            infer.left.height = obj.height;

            infer.name = m_d->detector->labelName(obj.label);
            infers.push_back(infer);
        }

        return (int)infers.size();
    }

    void Infer::extractFeatures(const cv::Mat &img, const pic::dnn::DetectedObjList &objs, std::vector<ce::MonoObj> &monos)
	{
		for (int i = 0; i < objs.size(); ++i) {
			ce::MonoObj mono;

			float x1 = (objs[i].x < 0) ? 0 : objs[i].x;
	        float y1 = (objs[i].y < 0) ? 0 : objs[i].y;
	        float x2 = ((objs[i].x + objs[i].width) > img.cols) ? img.cols : (objs[i].x + objs[i].width);
	        float y2 = ((objs[i].y + objs[i].height) > img.rows) ? img.rows : (objs[i].y + objs[i].height);
	        float w = x2 - x1;
	        float h = y2 - y1;
			
			mono.x = x1;
			mono.y = y1;
			mono.width = w;
			mono.height = h;
            mono.label = objs[i].label;
            mono.score=objs[i].score;

            cv::Mat roi;
            img(cv::Rect(cv::Point((int)x1, (int)y1), cv::Point((int)x2, (int)y2))).copyTo(roi);
#if CE_USE_IMAGENET_FEATURE
            m_d->classifier->getFeatures(roi, mono.features);
#elif CE_USE_RANGE_FINDER
            getFeatures(roi, mono.features);
#endif
#if CE_USE_IMAGENET_FEATURE || CE_USE_RANGE_FINDER
            //appendSizeFeatures(img.cols, img.rows, w, h, mono.features);
#endif
            monos.push_back(mono);
		}
	}

#if CE_USE_RANGE_FINDER
    int Infer::getFeatures(const cv::Mat &roi, std::vector<float> &feats)
    {
        const int smallSize = 16;
        const int step = 8;
        const int maxVal = 256 / step;

        feats.clear();

        assert(roi.channels() == 3);
        cv::Mat smallRoi;
        cv::resize(roi, smallRoi, cv::Size(smallSize, smallSize));

        int featsLen = smallSize*smallSize * 3;

        feats.resize(featsLen);
        for (int y = 0; y < smallRoi.rows; y++) {
            uchar *ptr = smallRoi.ptr<uchar>(y);
            for (int x = 0; x < smallRoi.cols; x++) {
                for (int c = 0; c < 3; c++) {
                    int val = (int)ptr[x*3+c];
                    int i = c*smallSize*smallSize + y*smallSize + x;
                    val /= 8;
                    feats[i] = (float)val / maxVal;
                }
            }
        }

        return (int)feats.size();
    }
#endif

	int Infer::appendSizeFeatures(int imgWidth, int imgHeight, int roiWidth, int roiHeight, std::vector<float> &feats)
	{
		const int featsLen = 232;
        float featMin = -1, featMax = 1;

        for (int i = 0; i < (int)feats.size(); i++) {
            if (i == 0)
                featMin = featMax = feats[i];
            else if (featMin>feats[i])
                featMin = feats[i];
            else if (featMax < feats[i])
                featMax = feats[i];
        }
		for (int i = 0; i < featsLen; i += 2) {
			feats.push_back((featMax-featMin) * roiWidth / imgWidth);
            feats.push_back((featMax - featMin) * roiHeight / imgHeight);
		}

		return (int)feats.size();
	}

    int Infer::createBinoObj(std::vector<ce::MonoObj>& monos, int64 time, std::vector<ce::BinoObj>& objs)
    {
        objs.clear();
        for (int i = 0; i < (int)monos.size(); i++) {
            ce::BinoObj obj;
            obj.left = monos[i];
            obj.frameTime = time;
            objs.push_back(obj);
        }
        return (int)objs.size();
    }

    void Infer::createInferList(BinoObjList &binoObjs, std::vector<InferInfo> &infers)
    {
        for (int i = 0; i < binoObjs.size(); ++i) {
            ce::BinoObj bino = binoObjs[i];
            ce::InferInfo infer;
            if (bino.distance < 0.01)
                continue;
            infer.id = bino.id;
            infer.left.x = bino.left.x;
            infer.left.y = bino.left.y;
            infer.left.width = bino.left.width;
            infer.left.height = bino.left.height;
            infer.distance = bino.distance;
            infer.velocity = bino.velocity;
            infer.name = m_d->detector->labelName(bino.left.label);
            infers.push_back(infer);
        }
    }

#if CE_USE_STEREO_CALIB
    bool Infer::initStereoCalib()
    {
        std::string fname = fs::locateFile(m_d->cameraParam, "");
        if (!m_d->camera->setParam(fname))
            return false;
        return true;
    }

    static void getRectPoints(const std::vector<cv::KeyPoint> &keyPoints, const std::vector<cv::Rect2f> &rects, BinoObjList &objs)
    {
        for (size_t i = 0; i < keyPoints.size(); i++) {
            cv::Point2f pt;
            pt = keyPoints[i].pt;

            for (size_t ri = 0; ri < rects.size(); ri++) {
                cv::Rect2f rc = rects[ri];
                if (rc.contains(pt)) {
                    objs[ri].left.keypnts.push_back(keyPoints[i]);
                    break;
                }
            }
        }
    }

    void Infer::calcDistances(cv::Mat &left_img, cv::Mat &right_img, BinoObjList &objs)
    {
        std::vector<cv::KeyPoint> keyPoints;
        m_d->featureDetector->detect(left_img, keyPoints);

        std::vector<cv::Rect2f> rects;
        std::vector<double> distances;

        for (int i = 0; i < (int)objs.size(); i++) {
            BinoObj &obj = objs[i];
            cv::Rect2f rc;
            rc.x = obj.left.x;
            rc.y = obj.left.y;
            rc.width = obj.left.width;
            rc.height = obj.left.height;
            rects.push_back(rc);
        }

        m_d->camera->calcDistances(left_img, right_img, rects, keyPoints, distances);

        getRectPoints(keyPoints, rects, objs);
        for (size_t i = 0; i < objs.size(); i++)
            objs[i].distance = distances[i];
    }
#endif
}
