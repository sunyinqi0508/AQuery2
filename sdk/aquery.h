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

struct Context{
    typedef int (*printf_type) (const char *format, ...);
	void* module_function_maps = 0;
	Config* cfg;

	int n_buffers, *sz_bufs;
	void **buffers;

	void* alt_server;
	Log_level log_level = LOG_INFO;

	Session current;


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

extern void* Aalloc(unsigned long long sz);
extern void Afree(void * mem);
extern void register_memory(void* ptr, deallocator_t deallocator);

#endif