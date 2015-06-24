#include "rwmutex.hpp"

std::chrono::microseconds rwmutex::waitTime {100};

void rwmutex::read_lock() {
	accessor.lock();
	while (writing) { std::this_thread::sleep_for(waitTime); }
	readnum++;
	accessor.unlock();
}

void rwmutex::read_unlock() {
	readnum--;
}

void rwmutex::write_lock() {
	accessor.lock();
	while (writing || readnum > 0) {
		std::this_thread::sleep_for(waitTime);
	}
	writing.store(true);
	accessor.unlock();
}

void rwmutex::write_unlock() {
	writing.store(false);
}

bool rwmutex::is_write_locked() {
	return writing;
}
