#pragma once
#include <vector_type>
#include <utility>
#include <thread>
#include <chrono>
class GC {
	template<class T>
	using vector = vector_type<T>;
	template<class ...T>
	using tuple = std::tuple<T...>;
	size_t current_size, max_size, interval, forced_clean;
	bool running, alive;
//  ptr, dealloc, ref, sz
	vector<tuple<void*, void (*)(void*)>> q;
	std::thread handle;
	void gc()
	{
		
	}
	void reg(void* v, uint32_t ref, uint32_t sz, 
		void(*f)(void*) = [](void* v) {free (v); }) {
		current_size += sz;
		if (current_size > max_size)
			gc();
		q.push_back({ v, f });
	}
	void daemon() {
		using namespace std::chrono;
		while (alive) {
			if (running) {
				gc();
				std::this_thread::sleep_for(microseconds(interval));
			}
			else {
				std::this_thread::sleep_for(10ms);
			}
		}
	}
	void start_deamon() {
		handle = std::thread(&daemon);
		alive = true;
	}
	void terminate_daemon() {
		running = false;
		alive = false;
		using namespace std::chrono;

		if (handle.joinable()) {
			std::this_thread::sleep_for(microseconds(1000 + std::max(static_cast<size_t>(10000), interval)));
			handle.join();
		}
	}
};