#pragma once
#include "types.h"
#include <cstdio>
#include <string>
#include <limits>
#include <cstring>
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

#ifdef __SIZEOF_INT128__
inline const char* get_int128str(__int128_t v, char* buf){
	bool neg = false;
	if (v < 0) { 
		if(v == std::numeric_limits<__int128_t>::min())
			return "-170141183460469231731687303715884105728";
		v = -v; 
		neg = true; 
	}
	do {
		*--buf = v%10 + '0';
		v /= 10;
	} while(v);
	if (neg) *--buf = '-';
    return buf;
}

inline const char* get_uint128str(__uint128_t v, char* buf){
	do {
		*--buf = v%10 + '0';
		v /= 10;
	} while(v);
	return buf;
}
extern char* gbuf;

void setgbuf(char* buf = 0);

template<class T>
inline decltype(auto) printi128(const T& v){
    return v;
}

template<>
inline decltype(auto) printi128<__int128_t>(const __int128_t& v) {
    *(gbuf+=40) = 0;
    return get_int128str(v, gbuf++);
}

template<>
inline decltype(auto) printi128<__uint128_t>(const __uint128_t& v) {
    *(gbuf+=40) = 0;
    return get_uint128str(v, gbuf++);
}

#else
#define printi128(x) x
#endif
