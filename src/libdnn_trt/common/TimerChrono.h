#ifndef __TIMER_CHRONO__
#define __TIMER_CHRONO__

#include <chrono>
class Timer_CHRONO {
public:
	typedef std::chrono::high_resolution_clock Clock;
	/*! \brief start or restart timer */
	inline void Tic() {
		start_ = Clock::now();
	}
	/*! \brief stop timer */
	inline void Toc() {
		end_ = Clock::now();
	}
	/*! \brief return time in ms */
	inline __int64 Elapsed() {
		auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_ - start_);
		return duration.count();
	}

private:
	std::chrono::high_resolution_clock::time_point start_, end_;
};

#endif //__TIMER_CHRONO__
