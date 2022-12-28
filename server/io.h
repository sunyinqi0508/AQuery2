#pragma once
#include "types.h"
#include <cstdio>
#include <string>
#include <limits>
#include <cstring>
#include <string_view>
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

template<class T>
inline decltype(auto) print_hook(const T& v){
    return v;
}

template<>
inline decltype(auto) print_hook<bool>(const bool& v) {
	return v? "true" : "false";
}

template<>
inline decltype(auto) print_hook<std::string_view>(const std::string_view& v) {
	return v.data();
}

extern char* gbuf;

void setgbuf(char* buf = 0);

template<>
inline decltype(auto) print_hook<types::date_t>(const types::date_t& v) {
	*(gbuf += types::date_t::string_length()) = 0;

	return v.toString(gbuf);
}
template<>
inline decltype(auto) print_hook<types::time_t>(const types::time_t& v) {
	*(gbuf += types::time_t::string_length()) = 0;
	return v.toString(gbuf);
}
template<>
inline decltype(auto) print_hook<types::timestamp_t>(const types::timestamp_t& v) {
	*(gbuf += types::timestamp_t::string_length()) = 0;
	return v.toString(gbuf);
}
#ifdef __AQ__HAS__INT128__
constexpr struct __int128__struct{
    uint64_t low, high;
	// constexpr bool operator==(__int128_t x) const{
    //     return (x>>64) == high and (x&0xffffffffffffffffull) == low;
    // }
	bool operator==(__int128_t x) const{
		return *((const __int128_t*) this) == x;
	}
}__int128_max_v = {0x0000000000000000ull, 0x8000000000000000ull};

inline const char* get_int128str(__int128_t v, char* buf){
	bool neg = false;
	if (v < 0) { 
		if(__int128_max_v == v)
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


template<>
inline decltype(auto) print_hook<__int128_t>(const __int128_t& v) {
    *(gbuf+=40) = 0;
    return get_int128str(v, gbuf++);
}

template<>
inline decltype(auto) print_hook<__uint128_t>(const __uint128_t& v) {
    *(gbuf+=40) = 0;
    return get_uint128str(v, gbuf++);
}

#else

#endif
