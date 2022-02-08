#ifndef PSTDTHREAD_H
#define PSTDTHREAD_H

#include <thread>
#include <mutex>
#include <chrono>

#ifndef WIN32
#include <limits.h>
#endif

class PStdThreadCallback
{
public:
	virtual void sendMessage(int msg, long param = 0, void *data = NULL) = 0;
};

class PStdThread
{
public:
    PStdThread()
	{
		m_quit = false;
		m_pause = false;
        m_cond = -1;

	}
	virtual ~PStdThread() {
        stop();
    }

	static void destroy(PStdThread **thread)
	{
		PStdThread *t = *thread;
		if (t) {
			t->stop();
			delete t;
			*thread = NULL;
		}
	}

    static void msleep(int ms) {
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    }

    bool start() {
        if (m_thread.joinable())
            return false;
        m_thread = std::thread(&PStdThread::run, this);
        return true;
    }

    void stop()
    {
        m_mutex.lock();
        m_pause = false;
        m_quit = true;
        m_mutex.unlock();

        if (m_thread.joinable())
            m_thread.join();
    }

	void pause()
	{
		m_mutex.lock();
		m_pause = true;
		m_mutex.unlock();
		wake();
	}

	void resume()
	{
		m_mutex.lock();
		m_pause = false;
		m_mutex.unlock();
		wake();
	}
    
    void initCond() { m_cond = 0; }

    void setCond(int cond) {
        std::lock_guard<std::mutex> locker(m_mutex);
        if (m_cond >= 0)
            m_cond = cond;
    }
	void waitCond(unsigned long time = ULONG_MAX)
	{
        while (!m_quit && m_cond > 0)
            std::this_thread::sleep_for(std::chrono::microseconds(1));
	}

	void wake()
	{
        std::lock_guard<std::mutex> locker(m_mutex);
        if (m_cond > 0)
            m_cond = 0;
	}

	bool quited()
	{
		std::lock_guard<std::mutex> locker(m_mutex);
		return m_quit;
	}

	bool paused()
	{
        std::lock_guard<std::mutex> locker(m_mutex);
        return m_pause;
	}

	void setCallback(PStdThreadCallback *cb, int regist=1)
	{
        std::lock_guard<std::mutex> locker(m_mutex);

        if (regist) {
			m_callbacks.push_back(cb);
		}
		else {
			for (int i = 0; i < (int)m_callbacks.size(); ++i) {
				if (m_callbacks[i] == cb) {
					m_callbacks.erase(m_callbacks.begin() + i);
					break;
				}
			}
		}
	}

	void sendMessage(int msg, long param, void *data)
	{
        std::lock_guard<std::mutex> locker(m_mutex);
        
        for (int i = 0; i < (int)m_callbacks.size(); ++i) {
			if (m_callbacks[i])
				m_callbacks[i]->sendMessage(msg, param, data);
		}
	}

protected:
    virtual void run() = 0;

	bool m_quit;
	bool m_pause;
	std::mutex m_mutex;
    std::thread m_thread;
    int m_cond;

	std::vector<PStdThreadCallback*> m_callbacks;
};

class PStdTimer {
	using Clock = std::chrono::high_resolution_clock;
public:
	/*! \brief start or restart timer */
	inline void tic() {
		m_start = Clock::now();
	}
    inline void Tic() {
        return tic();
    }
	/*! \brief stop timer */
    inline long long toc() {
		m_end = Clock::now();
        return elapsed();
	}
    inline long long Toc() {
        return toc();
    }
	/*! \brief return time in ms */
	inline long long elapsed() {
		auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(m_end - m_start);
		return duration.count();
	}
    inline long long Elapsed() {
        return elapsed();
    }

private:
	Clock::time_point m_start, m_end;
};

#endif // PSTDTHREAD_H
