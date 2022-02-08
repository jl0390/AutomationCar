#ifndef __CAR_EYE_MATH_HPP__
#define __CAR_EYE_MATH_HPP__

#include <map>

namespace ce {
	typedef std::map<int, unsigned int> IntMap;

	static int getFrequentlyElement(const IntMap& in) {
		int count = 0;
		int res;
		for (IntMap::const_iterator iter = in.begin(); iter != in.end(); ++iter) {
			if (iter->second > count) {
				res = iter->first;
				count = iter->second;
			}
		}
		return res;
	}

	static float L2distance(const std::vector<float> &x, const std::vector<float> &y)
	{
		float sum = 0;
		if (x.size() != y.size()) {
			std::cerr << "Two vector's dimensions are not same." << std::endl;
			//return FLT_MAX;
		}
		for (int i = 0; i < x.size(); ++i) {
			sum += std::pow(x[i] - y[i], 2.0f);
		}
		sum = std::sqrt(sum);
		return sum;
	}

	static double calcDistance(double cams, double focus, double delta, double lx, double rx)
	{
		return (cams * focus / std::abs(lx - rx + delta));
	}


}
#endif //__CAR_EYE_MATH_HPP__