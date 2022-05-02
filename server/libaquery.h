#ifndef _AQUERY_H
#define _AQUERY_H

#include "table.h"
#include <unordered_map>

enum Log_level {
	LOG_INFO,
	LOG_ERROR,
	LOG_SILENT
};

struct Context{
    typedef int (*printf_type) (const char *format, ...);
	std::unordered_map<const char*, void*> tables;
    std::unordered_map<const char*, uColRef *> cols;
	Log_level log_level = LOG_SILENT;
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

#ifdef _MSC_VER
#define __DLLEXPORT__  __declspec(dllexport) __stdcall 
#else 
#define __DLLEXPORT__
#endif

#endif