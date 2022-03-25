#ifndef _TYPES_H
#define _TYPES_H
#include <typeinfo>
#include <functional>
#include <cstdint>
#include <type_traits>
#include <string> 	
#ifdef _MSC_VER
#define __restrict__ __restrict
#endif
namespace types {
	enum Type_t {
		AINT, AFLOAT, ASTR, ADOUBLE, ALDOUBLE, ALONG, ASHORT, ADATE, ATIME, ACHAR,
		AUINT, AULONG, AUSHORT, AUCHAR, NONE, ERROR
	};
	static constexpr const char* printf_str[] = { "%d ", "%f ", "%s ", "%lf ", "%llf ", "%ld ", "%hi ", "%s ", "%s ", "%c ",
		"%u ", "%lu ", "%hu ", "%hhu ", "NULL " };
	// TODO: deal with data/time <=> str/uint conversion
	struct date_t {
		uint32_t val;
		date_t(const char* d) {

		}
		std::string toString() const;
	};
	struct time_t {
		uint32_t val;
		time_t(const char* d) {

		}
		std::string toString() const;
	};
	template <typename T>
	struct Types {
		typedef T type;
		constexpr Types() noexcept = default;
#define ConnectTypes(f) \
		f(int, AINT) \
		f(float, AFLOAT) \
		f(const char*, ASTR) \
		f(double, ADOUBLE) \
		f(long double, ALDOUBLE) \
		f(long, ALONG) \
		f(short, ASHORT) \
		f(date_t, ADATE) \
		f(time_t, ATIME) \
		f(unsigned char, ACHAR) \
		f(unsigned int, AUINT) \
		f(unsigned long, AULONG) \
		f(unsigned short, AUSHORT) \
		f(unsigned char, AUCHAR) 

		constexpr static Type_t getType() {
#define	TypeConnect(x, y) if(typeid(T) == typeid(x)) return y; else
			ConnectTypes(TypeConnect)
				return NONE;
		}
		//static constexpr inline void print(T& v);
	};
#define ATypeSize(t, at) sizeof(t),
	static constexpr size_t AType_sizes[] = { ConnectTypes(ATypeSize) 1 };
#define Cond(c, x, y) typename std::conditional<c, x, y>::type
#define Comp(o) (sizeof(T1) o sizeof(T2))
#define Same(x, y) (std::is_same_v<x, y>)
#define _U(x) std::is_unsigned<x>::value
#define Fp(x) std::is_floating_point<x>::value
	template <class T1, class T2>
	struct Coercion {
		using t1 = Cond(Comp(<= ), Cond(Comp(== ), Cond(Fp(T1), T1, Cond(Fp(T2), T2, Cond(_U(T1), T2, T1))), T2), T1);
		using t2 = Cond(Same(T1, T2), T1, Cond(Same(T1, const char*) || Same(T2, const char*), const char*, void));
		using type = Cond(Same(t2, void), Cond(Same(T1, date_t) && Same(T2, time_t) || Same(T1, time_t) && Same(T2, time_t), void, t1), t2);
	};
#define __Eq(x) (sizeof(T) == sizeof(x))
	template<class T>
	struct GetFPType {
		using type = Cond(__Eq(float), float, Cond(__Eq(double), double, long double));
	};
	template<class T>
	struct GetLongType
	{
		using type = Cond(_U(T), unsigned long long, Cond(Fp(T), long double, long long));
	};
}
#endif // !_TYPES_H
