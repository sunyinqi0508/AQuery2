#pragma once

#include <ctime>
#include <type_traits>
#include<string>

#if ((defined(_MSVC_LANG) && _MSVC_LANG >= 201703L) || __cplusplus >= 201703L)
constexpr static bool cpp_17 = true;
#else
constexpr static bool cpp_17 = false;
#endif
template <class T>
inline const char* str(const T& v) {
	return "";
}

template <class T>
constexpr char* aq_itoa(T t, char* buf){
	if constexpr (std::is_signed<T>::value){
		if (t < 0){
			*buf++ = '-';
			t = -t;
		}
	}
	while(t > 0){
		*buf++ = t%10 + '0';
		t /= 10;
	}
	return buf;
}

extern std::string base62uuid(int l = 6);
