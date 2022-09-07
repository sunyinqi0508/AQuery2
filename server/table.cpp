#include "table.h"


#ifdef __SIZEOF_INT128__
template <>
void print<__int128_t>(const __int128_t& v, const char* delimiter){
	char s[41];
	s[40] = 0;
	std::cout<< get_int128str(v, s+40)<< delimiter;
}
template <>
void print<__uint128_t>(const __uint128_t&v, const char* delimiter){
	char s[41];
	s[40] = 0;
    std::cout<< get_uint128str(v, s+40) << delimiter;
}
std::ostream& operator<<(std::ostream& os, __int128 & v)
{
	print(v);
	return os;
}
std::ostream& operator<<(std::ostream& os, __uint128_t & v)
{
	print(v);
	return os;
}
#endif

template <>
void print<bool>(const bool&v, const char* delimiter){
	std::cout<< (v?"true":"false") << delimiter;
}
