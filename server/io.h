#pragma once
#include "types.h"
#include <cstdio>
#include <string>
#include <cstdio>
#include <string>
template <class ...Types>
std::string generate_printf_string(const char* sep = " ", const char* end = "\n") {
	std::string str;
	(void)std::initializer_list<int>{
		(str += types::printf_str[types::Types<Types>::getType()], str += sep, 0)...
	};
	str += end;
	return str;
}
