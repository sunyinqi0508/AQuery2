#include "io.h"

char* gbuf = 0;

void setgbuf(char* buf){
	static char* b = 0;
	if (buf == 0)
		gbuf = b;
	else
		gbuf = buf;
}	

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


#include <chrono>
#include <ctime>
namespace types {
	using namespace std;
	using namespace chrono;
	string date_t::toString() const {
		uint32_t curr_v = val;
		tm;
		time_t;
		return string() + string("/") + string() + string("/") + string();
	}
	string time_t::toString() const {
		uint32_t curr_v = val;
		
		return string() + string("/") + string() + string("/") + string();
	}
}

#include "utils.h"
#include <random>
using std::string;
string base62uuid(int l = 8) {
    using namespace std;
    constexpr static const char* base62alp = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    static mt19937_64 engine(chrono::system_clock::now().time_since_epoch().count());
    static uniform_int_distribution<uint64_t> u(0x10000, 0xfffff);
    uint64_t uuid = (u(engine) << 32ull) + (chrono::system_clock::now().time_since_epoch().count() & 0xffffffff);
    printf("%p\n", uuid);
    string ret;
    while (uuid && l-- >= 0) {
        ret = string("") + base62alp[uuid % 62] + ret;
        uuid /= 62;
    }
    return ret;
}


template<typename _Ty>
inline void vector_type<_Ty>::out(uint32_t n, const char* sep) const
{
	n = n > size ? size : n;
	std::cout << '(';
	{	
		uint32_t i = 0;
		for (; i < n - 1; ++i)
			std::cout << this->operator[](i) << sep;
		std::cout << this->operator[](i);
	}
	std::cout << ')';
}
