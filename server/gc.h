#pragma once
#include <atomic>
#ifndef QUERY_DECLSPEC
#define QUERY_DECLSPEC
#endif

class QUERY_DECLSPEC ScratchSpace {
public:
	void* ret;
	char* scratchspace;
	size_t ptr;
	size_t cnt;
	size_t capacity;
	size_t initial_capacity;
	void* temp_memory_fractions;
	
	//uint8_t status; 
	// record maximum size
	constexpr static uint8_t Grow = 0x1;
	// no worry about overflow
	constexpr static uint8_t Use = 0x0; 

	void init(size_t initial_capacity);

	// apply for memory
	void* alloc(uint32_t sz);

	void register_ret(void* ret);

	// reorganize memory space
	void release();

	// reset status of the scratch space 
	void reset();

	// reset scratch space to initial capacity.
	void cleanup();
};


#ifndef __AQ_USE_THREADEDGC__
class QUERY_DECLSPEC GC {
private:
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
	using gc_deallocator_t = void (*)(void*);
	// maybe use volatile std::thread::id instead
protected:
	void acquire_lock();
	void release_lock();
	void gc();
	void daemon();
	void start_deamon();
	void terminate_daemon();

public:
	ScratchSpace scratch;
	void reg(void* v, uint32_t sz = 0xffffffff, 
			void(*f)(void*) = free
		);

	uint32_t get_threshold() const {
		return threshould;
	}

	GC(
		uint64_t max_size = 0xfffffff, uint32_t max_slots = 4096, 
		uint32_t interval = 10000, uint32_t forced_clean = 1000000,
		uint32_t threshould = 64, //one seconds
		uint32_t scratch_sz = 0x1000000 // 16 MB
	) : max_size(max_size), max_slots(max_slots), 
		interval(interval), forced_clean(forced_clean), 
		threshould(threshould) {

		start_deamon();
		GC::gc_handle = this;
		this->scratch.init(1);
	} // 256 MB

	~GC(){
		terminate_daemon();
		scratch.cleanup();
	}

	static GC* gc_handle;
	static ScratchSpace *scratch_space;
	template <class T>
	static inline gc_deallocator_t _delete(T*) {
		return [](void* v){
			delete (T*)v;
		};
	} 
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
