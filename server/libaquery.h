#ifndef _AQUERY_H
#define _AQUERY_H

#include "table.h"
#include <unordered_map>

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

struct Context{
    typedef int (*printf_type) (const char *format, ...);
	std::unordered_map<const char*, void*> tables;
    std::unordered_map<const char*, uColRef *> cols;

	Config* cfg;

	int n_buffers, *sz_bufs;
	void **buffers;
	
	void* alt_server;
	Log_level log_level = LOG_INFO;
	printf_type print = printf;

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
};

#ifdef _WIN32
#define __DLLEXPORT__  __declspec(dllexport) __stdcall 
#else 
#define __DLLEXPORT__
#endif

#define __AQEXPORT__(_Ty) extern "C" _Ty __DLLEXPORT__ 
#endif