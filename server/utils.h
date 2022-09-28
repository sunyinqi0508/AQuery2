#pragma once
#include <ctime>
#if ((defined(_MSVC_LANG) && _MSVC_LANG >= 201703L) || __cplusplus >= 201703L)
constexpr static bool cpp_17 = true;
#else
constexpr static bool cpp_17 = false;
#endif
template <class T>
inline const char* str(const T& v) {
	return "";
}

#include<string>
extern std::string base62uuid(int l = 6);