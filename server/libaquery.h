#ifndef _AQUERY_H
#define _AQUERY_H

#ifdef __INTELLISENSE__
	#define __AQUERY_ITC_USE_SEMPH__
	#define THREADING
#endif

#include <unordered_map>
#include <chrono>
#include <filesystem>
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

struct StoredProcedure{
	uint32_t cnt, postproc_modules;
	char **queries;
	const char* name;
	void **__rt_loaded_modules;
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
	const char* aquery_root_path;
#ifdef THREADING
	void* thread_pool;
#endif	
#ifndef __AQ_USE_THREADEDGC__
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
	std::unordered_map<std::string, void*> tables;
    std::unordered_map<std::string, uColRef *> cols;
    std::unordered_map<std::string, StoredProcedure> stored_proc;
};



#ifdef _WIN32
#define __DLLEXPORT__  __declspec(dllexport) __stdcall 
#else 
#define __DLLEXPORT__
#endif

#define __AQEXPORT__(_Ty) extern "C" _Ty __DLLEXPORT__ 
typedef void (*deallocator_t) (void*);


#include <type_traits>
#include "jeaiii_to_text.h"

template<class T>
inline std::enable_if_t<std::is_integral_v<T>, char *> 
aq_to_chars(void* value, char* buffer) { 
	return to_text(buffer, *static_cast<T*>(value));
}

template<class T>
inline std::enable_if_t<!std::is_integral_v<T>, char *> 
aq_to_chars(void* value, char* buffer) {
	return buffer;
}

#ifdef __SIZEOF_INT128__
template<>
inline char*
aq_to_chars<__int128_t>(void* value, char* buffer) {
    return jeaiii_i128<__int128_t>(buffer, *static_cast<__int128_t*>(value));
}

template<>
inline char*
aq_to_chars<__uint128_t>(void* value, char* buffer) {
    return jeaiii_i128<__uint128_t>(buffer, *static_cast<__uint128_t*>(value));
}
#endif

template<> char* aq_to_chars<float>(void* , char*);
template<> char* aq_to_chars<double>(void* , char*);
template<> char* aq_to_chars<char*>(void* , char*);
template<> char* aq_to_chars<types::date_t>(void* , char*);
template<> char* aq_to_chars<types::time_t>(void* , char*);
template<> char* aq_to_chars<types::timestamp_t>(void* , char*);
typedef int (*code_snippet)(void*);

template <class _This_Struct>
inline void AQ_ZeroMemory(_This_Struct& __val) {
	memset(&__val, 0, sizeof(_This_Struct));
}

#endif
