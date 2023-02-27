#ifndef _AQUERY_H
#define _AQUERY_H

enum Log_level {
	LOG_INFO,
	LOG_ERROR,
	LOG_SILENT
};

enum Backend_Type {
	BACKEND_AQuery,
	BACKEND_MonetDB,
	BACKEND_MariaDB
};

struct Config{
    int running, new_query, server_mode,
	 	backend_type, has_dll, n_buffers;
    int buffer_sizes[];
};

struct Session{
    struct Statistic{
        unsigned long long total_active;
        unsigned long long cnt_object;
        unsigned long long total_alloc;
    };
    void* memory_map;
};

struct Trigger;
struct IntervalBasedTriggerHost;
struct CallbackBasedTriggerHost;

struct Context {
    typedef int (*printf_type) (const char *format, ...);

	void* module_function_maps = nullptr;
	Config* cfg;

	int n_buffers, *sz_bufs;
	void **buffers;

	void* alt_server = nullptr;
	Log_level log_level = LOG_INFO;

	Session current;
	const char* aquery_root_path;
#ifdef THREADING
	void* thread_pool;
#endif	
#ifndef __AQ_USE_THREADEDGC__
	void* gc;
#endif
	printf_type print;
	Context();
	virtual ~Context();
	template <class ...Types>
	void log(Types... args) {
		if (log_level == LOG_INFO)
			print(args...);
	}
	template <class ...Types>
	void err(Types... args) {
		if (log_level <= LOG_ERROR)
			print(args...);
	}
	void init_session();
	void end_session();
	void* get_module_function(const char*);
    char remainder[]; 
};

#ifdef _WIN32
#define __DLLEXPORT__  __declspec(dllexport) __stdcall 
#else 
#define __DLLEXPORT__
#endif

#define __AQEXPORT__(_Ty) extern "C" _Ty __DLLEXPORT__ 

typedef void (*deallocator_t) (void*);
extern void default_deallocator(void* ptr);

extern void* Aalloc(unsigned long long sz, 
	deallocator_t deallocator = default_deallocator
);
extern void Afree(void * mem);
extern void register_memory(void* ptr,
 	deallocator_t deallocator = default_deallocator
);

__AQEXPORT__(void) init_session(Context* cxt);

#define __AQ_NO_SESSION__ __AQEXPORT__(void) init_session(Context*) {}

#ifdef _WIN32
#include <cstring>
#else 
namespace std {
	void* memcpy(void*, const void*, unsigned long long);
}
#endif

struct vectortype_storage{
	void* container = nullptr;
	unsigned int size = 0, capacity = 0;
	vectortype_storage(void* container, unsigned int size, unsigned int capacity) :
		container(container), size(size), capacity(capacity) {}
	vectortype_storage() = default;
	template <class Ty, template <typename> class VT>
	vectortype_storage(const VT<Ty>& vt) {
		std::memcpy(this, &vt, sizeof(vectortype_storage));
	}
};
struct ColRef_storage {
	void* container = nullptr;
	unsigned int size = 0, capacity = 0;
	const char* name = nullptr;
	int ty = 0; // what if enum is not int?
	ColRef_storage(void* container, unsigned int size, unsigned int capacity, const char* name, int ty) :
		container(container), size(size), capacity(capacity), name(name), ty(ty) {}
	ColRef_storage() = default;
	template <class Ty, template <typename> class VT>
	ColRef_storage(const VT<Ty>& vt) {
		std::memcpy(this, &vt, sizeof(ColRef_storage));
	}
};
#endif
