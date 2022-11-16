#ifndef __AQ_USE_THREADEDGC__
#include <atomic>
class GC {
private:;

	size_t max_slots, 
		   interval, forced_clean, 
		   forceclean_timer = 0;
	uint64_t max_size;
	bool running, alive;
//  ptr, dealloc, ref, sz
	uint32_t threshould;
	void *q, *q_back;
	void* handle;
	std::atomic<uint32_t> slot_pos;
	std::atomic<uint32_t> alive_cnt;
	std::atomic<uint64_t> current_size;
	volatile bool lock;
	// maybe use volatile std::thread::id instead
protected:
	void acquire_lock();
	void release_lock();
	void gc();
	void daemon();
	void start_deamon();
	void terminate_daemon();

public:
	void reg(void* v, uint32_t sz = 1, 
			void(*f)(void*) = free
		);

	GC(
		uint64_t max_size = 0xfffffff, uint32_t max_slots = 4096, 
		uint32_t interval = 10000, uint32_t forced_clean = 1000000,
		uint32_t threshould = 64 //one seconds
	) : max_size(max_size), max_slots(max_slots), 
		interval(interval), forced_clean(forced_clean), 
		threshould(threshould) {

		start_deamon();
		GC::gc_handle = this;
	} // 256 MB

	~GC(){
		terminate_daemon();
	}
	static GC* gc_handle;
    constexpr static void(*_free) (void*) = free;
};

#else
class GC {
public:
	GC(uint32_t) = default;
	void reg(
		void* v, uint32_t = 0, 
		void(*f)(void*) = free
	) const { f(v); }
	static GC* gc;
    constexpr static void(*_free) (void*) = free;
}
#endif
