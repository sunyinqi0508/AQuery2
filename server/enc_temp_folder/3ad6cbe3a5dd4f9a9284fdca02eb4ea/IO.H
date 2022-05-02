#pragma once
#include "types.h"
#include <cstdio>
#include <string>
template <class ...Types>
std::string generate_printf_string(const char* sep = " ", const char* end = "\n") {
	std::string str;
	((str += types::printf_str[types::Types<value_type_r<Types>>::getType()], str += sep), ...);
	const auto trim = str.size() - strlen(sep);
	if (trim > 0)
		str.resize(trim);
	str += end;
	return str;
}

