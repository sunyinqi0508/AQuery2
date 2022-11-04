#pragma once
#include <vector_type>
#include <utility>
#include <thread>
#include <chrono>
#include <atomic>
#ifndef __AQ_USE_THREADEDGC__
class GC {
private:
	template<class T>
	using vector = vector_type<T>;
	template<class ...T>
	using tuple = std::tuple<T...>;
	size_t current_size = 0, max_size, 
		   interval, forced_clean, 
		   forceclean_timer = 0;
	bool running, alive;
//  ptr, dealloc, ref, sz
	vector<tuple<void*, void (*)(void*)>> *q, *q_back;
	std::thread handle;
	std::atomic<std::thread::id> lock;

protected:
	void acquire_lock(){
		auto this_pid = std::this_thread::get_id();
		while(lock != this_pid)
		{
			while(lock != this_pid && lock != std::thread::id()) {
				std::this_thread::sleep_for(std::chrono::milliseconds(0));
			}
			lock = this_pid;
		}
	}
	
	void release_lock(){
		lock = std::thread::id();
	}

	void gc()
	{
		if (q->size() == 0)
			return;
		auto t = q;
		acquire_lock();
		q = q_back;
		release_lock();
		for(const auto& t : *t) {
			std::get<1>(t)(std::get<0>(t));
		}
		t->clear();
		q_back = t;
		running = false;
		current_size = 0;
	}


	void daemon() {
		using namespace std::chrono;

		while (alive) {
			if (running) {
				if (current_size > max_size || 
					forceclean_timer > forced_clean) 
				{
					gc();
					forceclean_timer = 0;
				}
				std::this_thread::sleep_for(microseconds(interval));
				forceclean_timer += interval;
			}
			else {
				std::this_thread::sleep_for(10ms);
				forceclean_timer += 10000;
			}
		}
	}
	void start_deamon() {
		q = new vector<tuple<void*, void (*)(void*)>>();
		q_back = new vector<tuple<void*, void (*)(void*)>>();
		lock = thread::id();
		alive = true;
		handle = std::thread(&daemon);
	}

	void terminate_daemon() {
		running = false;
		alive = false;
		delete q;
		delete q_back;
		using namespace std::chrono;

		if (handle.joinable()) {
			std::this_thread::sleep_for(microseconds(1000 + std::max(static_cast<size_t>(10000), interval)));
			handle.join();
		}
	}
public:
	void reg(void* v, uint32_t sz = 1, 
			void(*f)(void*) = [](void* v) {free (v); }
		) {
		acquire_lock();
		current_size += sz;
		q.push_back({ v, f });
		running = true;
		release_lock()
	}

	GC(
		uint32_t max_size = 0xfffffff, uint32_t interval = 10000, 
		uint32_t forced_clean = 1000000 //one seconds
	) : max_size(max_size), interval(interval), forced_clean(forced_clean){
		start_deamon();
	} // 256 MB

	~GC(){
		terminate_daemon();
	}
};

#else
class GC {
public:
	GC(uint32_t) = default;
	void reg(
		void* v, uint32_t = 0, 
		void(*f)(void*) = [](void* v) {free (v); }
	) const { f(v); }
}
#endif
