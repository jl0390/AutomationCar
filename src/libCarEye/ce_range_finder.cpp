
#include "ce_range_finder.h"
#include "ce_math.hpp"
#include "pstdthread.h"

#if CE_USE_RANGE_FINDER

namespace ce {

	RangeFinder::RangeFinder(double cam_dist, double cam_focus, double delta, double threshold)
	{
		m_camDist = cam_dist;
		m_camFocus = cam_focus;
		m_delta = delta;
		m_threshold = threshold;

		m_divider = 10;
		if (!featDetector)
			featDetector = new FeatureDetector;

		if (!m_descMatcher)
			m_descMatcher = new DescriptorMatcher;

		m_leftKeypnts.clear();
        m_rightKeypnts.clear();
	}

	RangeFinder::~RangeFinder()
	{
		if (featDetector)
			delete featDetector;

		if (m_descMatcher)
			delete m_descMatcher;
		
		m_leftKeypnts.clear();
		m_rightKeypnts.clear();
	}

    int RangeFinder::findObjs(const cv::Mat &left, const cv::Mat &right,
        std::vector<MonoObj> &leftmos, std::vector<MonoObj> &rightmos, std::vector<BinoObj> &bis)
    {
        PStdTimer timer;
        timer.tic();
        // detect feature keypoints from left and right frame.
        m_leftKeypnts.clear();
        m_rightKeypnts.clear();
        featDetector->detectFeatures(left, m_leftKeypnts);
        featDetector->detectFeatures(right, m_rightKeypnts);
        timer.toc();
        int64 detectTime = timer.elapsed();

        timer.tic();
        // extract feature keypoints from object regions
#if 1
        extractFeatures(leftmos, m_leftKeypnts);
        extractFeatures(rightmos, m_rightKeypnts);
#else
        extractCenterFeatures(leftmos, m_leftKeypnts, left.cols, left.rows);
        extractCenterFeatures(rightmos, m_rightKeypnts, right.cols, right.rows);
#endif
        timer.toc();
        int64 extFeatTime = timer.elapsed();

        timer.tic();
        // extract description of feature keypoints
        extractDescriptors(leftmos, left);
        extractDescriptors(rightmos, right);
        timer.toc();
        int64 extDescTime = timer.elapsed();

        timer.tic();
        // make pair MonoObjects
        matchMonoObjs(leftmos, rightmos, bis);

        timer.toc();
        int64 matchMonoTime = timer.elapsed();
#if 0
        std::cout << "Compute Distance times detect: " << detectTime << "ms, ext_feat: " << extFeatTime <<
            "ms, ext_desk: " << extDescTime << "ms, match_mono: " << matchMonoTime << "ms" << std::endl;
#endif
        return (int)bis.size();
    }

	std::vector<BinoObj> RangeFinder::computeDistance(const cv::Mat &left, const cv::Mat &right,
		std::vector<MonoObj> &leftmos, std::vector<MonoObj> &rightmos, int64 time)
	{
		PStdTimer timer;
		timer.tic();
		// detect feature keypoints from left and right frame.
        m_leftKeypnts.clear();
        m_rightKeypnts.clear();
		featDetector->detectFeatures(left, m_leftKeypnts);
		featDetector->detectFeatures(right, m_rightKeypnts);
		timer.toc();
		int64 detectTime = timer.elapsed();

#if 0
        cv::Mat left1, right1;
        cv::drawKeypoints(left, m_leftKeypnts, left1, cv::Scalar(0, 255, 0));
        cv::drawKeypoints(right, m_rightKeypnts, right1, cv::Scalar(0, 255, 0));
        cv::imshow("left", left1);
        cv::imshow("right", right1);
#endif
		timer.tic();
		// extract feature keypoints from object regions
#if 1
		extractFeatures(leftmos, m_leftKeypnts);
		extractFeatures(rightmos, m_rightKeypnts);
#else
		extractCenterFeatures(leftmos, m_leftKeypnts, left.cols, left.rows);
        extractCenterFeatures(rightmos, m_rightKeypnts, right.cols, right.rows);
#endif
		timer.toc();
		int64 extFeatTime = timer.elapsed();

		timer.tic();
		// extract description of feature keypoints
		extractDescriptors(leftmos, left);
		extractDescriptors(rightmos, right);
		timer.toc();
		int64 extDescTime = timer.elapsed();

		timer.tic();
		// make pair MonoObjects
        std::vector<BinoObj> bis;
        matchMonoObjs(leftmos, rightmos, bis);

		timer.toc();
		int64 matchMonoTime = timer.elapsed();

		//calc distances
		double  dis = 0;
		for (int i = 0; i < bis.size(); ++i) {
			dis = calcDistances(bis[i]);
			bis[i].distance = dis;
			bis[i].frameTime = time;
		}

#if 0
		std::cout << "Compute Distance times detect: " << detectTime << "ms, ext_feat: " << extFeatTime << 
			"ms, ext_desk: " << extDescTime << "ms, match_mono: " << matchMonoTime << "ms" << std::endl;
#endif
		return bis;
	}

	void RangeFinder::extractFeatures(std::vector<MonoObj> &mos, const std::vector<cv::KeyPoint> &keypnts)
	{
		for (int i = 0; i < mos.size(); ++i) {
			cv::Rect rect(mos[i].x, mos[i].y, mos[i].width, mos[i].height);
			mos[i].keypnts = extractKeypoints(keypnts, rect);
		}
	}

	void RangeFinder::extractCenterFeatures(std::vector<MonoObj> &mos, const std::vector<cv::KeyPoint> &keypnts, int width, int height)
	{
		int ratio = 4;
        int wmax = width / ratio;
        int hmax = height / ratio;

		for (int i = 0; i < mos.size(); ++i) {
			int x = mos[i].x;
			int y = mos[i].y;
			int w = mos[i].width;
			int h = mos[i].height;
			if (w > wmax) {
                x = x + w / 2 - wmax / 2;
				w = wmax;
			}
			if (h > hmax) {
                y = y + h / 2 - hmax / 2;
                h = hmax;
			}
			cv::Rect rect(x, y, w, h);
			mos[i].keypnts = extractKeypoints(keypnts, rect);
		}
	}

	std::vector<cv::KeyPoint> RangeFinder::extractKeypoints(const std::vector<cv::KeyPoint> &keypnts, const cv::Rect &rect)
	{
		std::vector<cv::KeyPoint> res;

		std::vector<cv::Point2f> pnts;
		cv::KeyPoint::convert(keypnts, pnts);

		for (int i = 0; i < pnts.size(); ++i) {
			if (rect.contains(pnts[i]))
				res.push_back(keypnts[i]);
		}
		return res;
	}

	void RangeFinder::extractDescriptors(std::vector<MonoObj> &mos, const cv::Mat &img)
	{
		for (int i = 0; i < mos.size(); ++i) {
			featDetector->computeDescripts(img, mos[i].keypnts, mos[i].descripts);
		}
	}

    int RangeFinder::matchMonoObjs(const std::vector<MonoObj> &leftmos, const std::vector<MonoObj> &rightmos, std::vector<BinoObj> &bis)
	{
		PStdTimer timer;
		timer.tic();
		cv::Mat dismat(leftmos.size(), rightmos.size(), CV_32FC1);
		for (int i = 0; i < leftmos.size(); ++i) {
			for (int j = 0; j < rightmos.size(); ++j) {
				dismat.at<float>(i, j) = ce::L2distance(leftmos[i].features, rightmos[j].features);
			}
		}
		int pair_num = std::min(leftmos.size(), rightmos.size());
		timer.toc();
		int64 l2dist_time = timer.elapsed();

		timer.tic();
		for (int n = 0; n < pair_num; ++n) {
			double minval;
			int loc[2];
			cv::minMaxIdx(dismat, &minval, NULL, loc, NULL);
			//printf("RangeFinder::matchMonoObjs pair_num=%d/%d, minVal=%lf\n", n, pair_num, minval);
			if (minval <= m_threshold) {
				BinoObj bi;
				bi.left = leftmos[loc[0]];
				bi.right = rightmos[loc[1]];
				bi.matches = m_descMatcher->match(bi.left.descripts, bi.right.descripts);
				if (bi.matches.size() != 0)
					bis.push_back(bi);
				// In distmat Matrix, ith Rows elements are all set by FLT_MAX;
#if 0
				cv::Mat(dismat.row(loc[0]).rows, dismat.row(loc[0]).cols, dismat.row(loc[0]).type(), FLT_MAX).copyTo(dismat.row(loc[0]));
#else
                for (int r = 0; r < dismat.rows; r++) {
                    float *ptr = dismat.ptr<float>(r);
                    for (int c = 0; c < dismat.cols; c++) {
                        if (r == loc[0] || c == loc[1])
                            ptr[c] = FLT_MAX;
                    }
                }
#endif
			}
			else {
				break;
			}
		}
		timer.toc();
		int64 match_time = timer.elapsed();

#if 0
		std::cout << "matchMonoObjs Times l2dist: " << l2dist_time << "ms, match: " << match_time << "ms" << std::endl;
#endif
		return (int)bis.size();
	}

	double RangeFinder::calcDistances(const BinoObj &bi)
	{
		if (bi.matches.size() == 0)
			return 0;
        m_delta = 0; //by riks
		float ssum = 0;
		int lx, rx;
		// get most frequently distance

		ce::IntMap rr;
		for (int i = 0; i < bi.matches.size(); ++i) {
			lx = bi.left.keypnts[bi.matches[i].queryIdx].pt.x;
			rx = bi.right.keypnts[bi.matches[i].trainIdx].pt.x;
			if (lx - rx + m_delta == 0)
				continue;
			double dd = (ce::calcDistance(m_camDist, m_camFocus, m_delta, lx, rx) + m_divider / 2) / m_divider;
			rr[int(dd) * m_divider] += 1;
		}

		ssum = getFrequentlyElement(rr);
		ssum = ssum / 100;

		return ssum;
	}
}
#endif