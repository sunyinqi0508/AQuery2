#include "pch_msc.hpp"

#include "io.h"
#include "table.h"
#include <limits>

#include <chrono>
#include <ctime>

#include "utils.h"
#include <random>

char* gbuf = nullptr;

void setgbuf(char* buf) {
	static char* b = 0;
	if (buf == 0)
		gbuf = b;
	else {
		gbuf = buf;
		b = buf;
	}
}

#ifdef __AQ__HAS__INT128__

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

template<class T> 
T getInt(const char*& buf){
	T ret = 0;
	while(*buf >= '0' and *buf <= '9'){
		ret = ret*10 + *buf - '0';
		buf++;
	}
	return ret;
}
template<class T> 
char* intToString(T val, char* buf){

	while (val > 0){
		*--buf = val%10 + '0';
		val /= 10;
	}
	
	return buf;
}
void skip(const char*& buf){ 
	while(*buf && (*buf >'9' || *buf < '0')) buf++; 
}

namespace types {
	using namespace std;
	using namespace chrono;

	date_t::date_t(const char* str) { fromString(str); }
	date_t& date_t::fromString(const char* str)  {
		if(str) {
			skip(str);
			year = getInt<short>(str);
			skip(str);
			month = getInt<unsigned char>(str);
			skip(str);
			day = getInt<unsigned char>(str);
		}
		else{
			day = month = year = 0;
		}
		return *this;
	}
	bool date_t::validate() const{
		return year <= 9999 && month <= 12 && day <= 31;
	}
	
	char* date_t::toString(char* buf) const {
		// if (!validate()) return "(invalid date)";
		*--buf = 0;
		buf = intToString(day, buf);
		*--buf = '-';
		buf = intToString(month, buf);
		*--buf = '-';
		buf = intToString(year, buf);
		return buf;
	}
	bool date_t::operator > (const date_t& other) const {
		return year > other.year || (year == other.year && (month > other.month || (month == other.month && day > other.day)));
	}

	bool date_t::operator < (const date_t& other) const {
		return year < other.year || (year == other.year && (month < other.month || (month == other.month && day < other.day)));
	}

	bool date_t::operator >= (const date_t& other) const {
		return year >= other.year || (year == other.year && (month >= other.month || (month == other.month && day >= other.day)));
	}

	bool date_t::operator <= (const date_t& other) const {
		return year <= other.year || (year == other.year && (month <= other.month || (month == other.month && day <= other.day)));
	}

	bool date_t::operator == (const date_t& other) const {
		return year == other.year && month == other.month && day == other.day;
	}

	bool date_t::operator != (const date_t& other) const {
		return !operator==(other);
	}

	time_t::time_t(const char* str) { fromString(str); }
	time_t& time_t::fromString(const char* str)  {
		if(str) {
			skip(str);
			hours = getInt<unsigned char>(str);
			skip(str);
			minutes = getInt<unsigned char>(str);
			skip(str);
			seconds = getInt<unsigned char>(str);
			skip(str);
			ms = getInt<unsigned int> (str);
		}
		else {
			hours = minutes = seconds = ms = 0;
		}
		return *this;
	}
	
	char* time_t::toString(char* buf) const {
		// if (!validate()) return "(invalid date)";
		*--buf = 0;
		buf = intToString(ms, buf);
		*--buf = ':';
		buf = intToString(seconds, buf);
		*--buf = ':';
		buf = intToString(minutes, buf);
		*--buf = ':';
		buf = intToString(hours, buf);
		return buf;
	}
	bool time_t::operator > (const time_t& other) const {
		return hours > other.hours || (hours == other.hours && (minutes > other.minutes || (minutes == other.minutes && (seconds > other.seconds || (seconds == other.seconds && ms > other.ms)))));
	}
	bool time_t::operator< (const time_t& other) const {
		return hours < other.hours || (hours == other.hours && (minutes < other.minutes || (minutes == other.minutes && (seconds < other.seconds || (seconds == other.seconds && ms < other.ms)))));
	} 
	bool time_t::operator>= (const time_t& other) const {
		return hours >= other.hours || (hours == other.hours && (minutes >= other.minutes || (minutes == other.minutes && (seconds >= other.seconds || (seconds == other.seconds && ms >= other.ms)))));
	}
	bool time_t::operator<= (const time_t& other) const{
		return hours <= other.hours || (hours == other.hours && (minutes <= other.minutes || (minutes == other.minutes && (seconds <= other.seconds || (seconds == other.seconds && ms <= other.ms)))));
	}
	bool time_t::operator==(const time_t& other) const {
		return hours == other.hours && minutes == other.minutes && seconds == other.seconds && ms == other.ms;
	}
	bool time_t::operator!= (const time_t& other) const {
		return !operator==(other);
	}
	bool time_t::validate() const{
		return hours < 24 && minutes < 60 && seconds < 60 && ms < 1000000;
	}

	timestamp_t::timestamp_t(const char* str) { fromString(str); }
	timestamp_t& timestamp_t::fromString(const char* str) {
		date.fromString(str);
		time.fromString(str);

		return *this;
	}
	bool timestamp_t::validate() const {
		return date.validate() && time.validate();
	}
	
	char* timestamp_t::toString(char* buf) const {
		buf = time.toString(buf);
		auto ret = date.toString(buf);
		*(buf-1) = ' ';
		return ret;
	}
	bool timestamp_t::operator > (const timestamp_t& other) const {
		return date > other.date || (date == other.date && time > other.time);
	}
	bool timestamp_t::operator < (const timestamp_t& other) const {
		return date < other.date || (date == other.date && time < other.time);
	}
	bool timestamp_t::operator >= (const timestamp_t& other) const {
		return date >= other.date || (date == other.date && time >= other.time);
	}
	bool timestamp_t::operator <= (const timestamp_t& other) const {
		return date <= other.date || (date == other.date && time <= other.time);
	}
	bool timestamp_t::operator == (const timestamp_t& other) const {
		return date == other.date && time == other.time;
	}
	bool timestamp_t::operator!= (const timestamp_t & other) const {
		return !operator==(other);
	}
}

template<class T>
void print_datetime(const T&v){
	char buf[T::string_length()];
	std::cout<<v.toString(buf + T::string_length());
}
std::ostream& operator<<(std::ostream& os, types::date_t & v)
{
	print_datetime(v);
	return os;
}
std::ostream& operator<<(std::ostream& os, types::time_t & v)
{
	print_datetime(v);
	return os;
}
std::ostream& operator<<(std::ostream& os, types::timestamp_t & v)
{
	print_datetime(v);
	return os;
}


using std::string;
string base62uuid(int l) {
    using namespace std;
    constexpr static const char* base62alp = 
		"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    static mt19937_64 engine(
		chrono::system_clock::now().time_since_epoch().count());
    static uniform_int_distribution<uint64_t> u(0x10000, 0xfffff);
    uint64_t uuid = (u(engine) << 32ull) + 
		(chrono::system_clock::now().time_since_epoch().count() & 0xffffffff);
    //printf("%llu\n", uuid);
    string ret;
    while (uuid && l-- >= 0) {
        ret = string("") + base62alp[uuid % 62] + ret;
        uuid /= 62;
    }
    return ret;
}


// template<typename _Ty>
// inline void vector_type<_Ty>::out(uint32_t n, const char* sep) const
// {
// 	n = n > size ? size : n;
// 	std::cout << '(';
// 	{	
// 		uint32_t i = 0;
// 		for (; i < n - 1; ++i)
// 			std::cout << this->operator[](i) << sep;
// 		std::cout << this->operator[](i);
// 	}
// 	std::cout << ')';
// }
