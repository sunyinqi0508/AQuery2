#ifndef _AQUERY_H
#define _AQUERY_H

#ifdef __INTELLISENSE__
	#define __AQUERY_ITC_USE_SEMPH__
	#define THREADING
	#define __AQ_THREADED_GC__
#endif

#include <unordered_map>
#include <chrono>
class aq_timer {
private:
	std::chrono::high_resolution_clock::time_point now;
public:
	aq_timer(){
		now = std::chrono::high_resolution_clock::now();
	}
	void reset(){
		now = std::chrono::high_resolution_clock::now();
	}
	long long elapsed(){
		long long ret = (std::chrono::high_resolution_clock::now() - now).count();
		reset();
		return ret;
	}
	long long lap() const{
		long long ret = (std::chrono::high_resolution_clock::now() - now).count();
		return ret;
	}
};

#include "table.h"


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

struct QueryStats{
	long long monet_time;
	long long postproc_time;
};
struct Config{
    int running, new_query, server_mode, 
	 	backend_type, has_dll, 
		n_buffers;
	QueryStats stats;
    int buffer_sizes[];
};

struct Session{
    struct Statistic{
        size_t total_active;
        size_t cnt_object;
        size_t total_alloc;
    } stats;
    void* memory_map;
};

struct Context{
    typedef int (*printf_type) (const char *format, ...);

	void* module_function_maps = 0;
	Config* cfg;

	int n_buffers, *sz_bufs;
	void **buffers;

	void* alt_server = 0;
	Log_level log_level = LOG_INFO;

	Session current;

#ifdef THREADING
	void* thread_pool;
#endif	
#ifdef __AQ_THREADED_GC__
	void* gc;
#endif
	printf_type print = &printf;
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
	std::unordered_map<const char*, void*> tables;
    std::unordered_map<const char*, uColRef *> cols;
};



#ifdef _WIN32
#define __DLLEXPORT__  __declspec(dllexport) __stdcall 
#else 
#define __DLLEXPORT__
#endif

#define __AQEXPORT__(_Ty) extern "C" _Ty __DLLEXPORT__ 
typedef void (*deallocator_t) (void*);

#endif

void test();