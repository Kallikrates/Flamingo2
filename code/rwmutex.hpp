#ifndef RWMUTEX_HPP
#define RWMUTEX_HPP

#include <chrono>
#include <atomic>
#include <mutex>
#include <thread>

class rwmutex {
public:
	rwmutex() {}
	virtual ~rwmutex() {}
	void read_lock();
	void read_unlock();
	void write_lock();
	void write_unlock();
private:
	static std::chrono::microseconds waitTime;
	std::atomic_bool writing {false};
	std::atomic<unsigned int> readnum {0};
	std::mutex accessor;
};

#endif //RWMUTEX_HPP
