#ifndef _TYPES_H
#define _TYPES_H
#include <cstdint>
#include <type_traits>
#include <tuple>

#if  defined(__SIZEOF_INT128__) and not defined(_WIN32)
#define __AQ__HAS__INT128__
#endif

#ifdef _MSC_VER
#define __restrict__ __restrict
#endif

template <class T>
constexpr static inline bool is_vector(const T&) {
	return false;
}
template <class T>
struct is_vector_impl : std::false_type {};
template <class T>
constexpr static bool is_vector_type = is_vector_impl<T>::value;

namespace types {
	enum Type_t {
		AINT32, AFLOAT, ASTR, ADOUBLE, ALDOUBLE, AINT64, AINT128, AINT16, ADATE, ATIME, AINT8,
		AUINT32, AUINT64, AUINT128, AUINT16, AUINT8, ABOOL, VECTOR, ATIMESTAMP, NONE, ERROR
	};
	static constexpr const char* printf_str[] = { "%d", "%f", "%s", "%lf", "%Lf", "%ld", "%d", "%hi", "%s", "%s", "%c",
		"%u", "%lu", "%s", "%hu", "%hhu", "%s", "%s", "Vector<%s>", "%s", "NULL", "ERROR" };
	static constexpr const char* SQL_Type[] = { "INT", "REAL", "TEXT", "DOUBLE", "DOUBLE", "BIGINT", "HUGEINT", "SMALLINT", "DATE", "TIME", "TINYINT",
		"INT", "BIGINT", "HUGEINT", "SMALLINT", "TINYINT", "BIGINT", "BOOL", "BIGINT", "TIMESTAMP", "NULL", "ERROR"};
	
	
	// TODO: deal with data/time <=> str/uint conversion
	struct date_t{
		unsigned char day = 0;
		unsigned char month = 0;
		short year = 0;

		date_t() = default;
		date_t(unsigned char day, unsigned char month, short year) : 
			day (day), month (month), year(year) {}
		date_t(const char*);
		date_t& fromString(const char*);
		bool validate() const;
		constexpr static unsigned string_length(){
			return 11;
		};
		char* toString(char* buf) const;
		bool operator > (const date_t&) const;
		bool operator < (const date_t&) const;
		bool operator >= (const date_t&) const;
		bool operator <= (const date_t&) const;
		bool operator == (const date_t&) const;
		bool operator != (const date_t&) const;
	};

	struct time_t{
		unsigned int ms = 0;
		unsigned char seconds = 0;
		unsigned char minutes = 0;
		unsigned char hours = 0;

		time_t() = default; 
		time_t(unsigned int ms, unsigned char seconds, unsigned char minutes, unsigned char hours) : 
			ms (ms), seconds (seconds), minutes(minutes), hours(hours) {}; 
		time_t(const char*); 
		time_t& fromString(const char*);
		bool validate() const;
		constexpr static unsigned string_length() {
			return 13;
		};
		char* toString(char* buf) const;
		bool operator > (const time_t&) const;
		bool operator < (const time_t&) const;
		bool operator >= (const time_t&) const;
		bool operator <= (const time_t&) const;
		bool operator == (const time_t&) const;
		bool operator != (const time_t&) const;
	};
	struct timestamp_t{
		date_t date;
		time_t time;
		timestamp_t() = default;
		timestamp_t(const date_t& d, const time_t& t) : date(d), time(t) {}
		timestamp_t(const char*); 
		timestamp_t& fromString(const char*);
		bool validate() const;
		constexpr static unsigned string_length(){
			return date_t::string_length() + time_t::string_length();
		};
		char* toString(char* buf) const;
		bool operator > (const timestamp_t&) const;
		bool operator < (const timestamp_t&) const;
		bool operator >= (const timestamp_t&) const;
		bool operator <= (const timestamp_t&) const;
		bool operator == (const timestamp_t&) const;
		bool operator != (const timestamp_t&) const;
	};
	template <typename T>
	struct Types {
		typedef T type;
		constexpr Types() noexcept = default;
#ifdef __AQ__HAS__INT128__
#define F_INT128(__F_) __F_(__int128_t, AINT128) \
				 __F_(__uint128_t, AUINT128)
#define ULL_Type __uint128_t
#define LL_Type __int128_t
#else
#define F_INT128(__F_) 
#define ULL_Type unsigned long long
#define LL_Type long long
#endif

#define ConnectTypes(f) \
		f(int, AINT32) \
		f(float, AFLOAT) \
		f(const char*, ASTR) \
		f(double, ADOUBLE) \
		f(long double, ALDOUBLE) \
		f(long, AINT64) \
		f(short, AINT16) \
		f(date_t, ADATE) \
		f(time_t, ATIME) \
		f(unsigned char, AINT8) \
		f(unsigned int, AUINT32) \
		f(unsigned long, AUINT64) \
		f(unsigned short, AUINT16) \
		f(unsigned char, AUINT8) \
		f(bool, ABOOL) \
		f(timestamp_t, ATIMESTAMP) \
		F_INT128(f)

		inline constexpr static Type_t getType() {
#define	TypeConnect(x, y) if constexpr(std::is_same<x, T>::value) return y; else
			ConnectTypes(TypeConnect)
			if constexpr (is_vector_type<T>) 
				return VECTOR;
			else
				return NONE;
		}
	};
#define ATypeSize(t, at) sizeof(t),
	static constexpr size_t AType_sizes[] = { ConnectTypes(ATypeSize) 1 };
#define Cond(c, x, y) typename std::conditional<c, x, y>::type
#define Comp(o) (sizeof(T1) o sizeof(T2))
#define Same(x, y) (std::is_same_v<x, y>)
#define __U(x) std::is_unsigned<x>::value
#define Fp(x) std::is_floating_point<x>::value
	template <class T1, class T2>
	struct Coercion {
		using t1 = Cond(Comp(<= ), Cond(Comp(== ), Cond(Fp(T1), T1, Cond(Fp(T2), T2, Cond(__U(T1), T2, T1))), T2), T1);
		using t2 = Cond(Same(T1, T2), T1, Cond(Same(T1, const char*) || Same(T2, const char*), const char*, void));
		using type = Cond(Same(t2, void), Cond(Same(T1, date_t) && Same(T2, time_t) || Same(T1, time_t) && Same(T2, time_t), void, t1), t2);
	};
#define __Eq(x) (sizeof(T) == sizeof(x))
	template<class T>
	struct GetFPTypeImpl {
		using type = Cond(__Eq(float), float, Cond(__Eq(double), double, double));
	};
	template<class T>
	using GetFPType = typename GetFPTypeImpl<typename std::decay<T>::type>::type;
	template<class T>
	struct GetLongTypeImpl {
		using type = Cond(__U(T), ULL_Type, Cond(Fp(T), double, LL_Type));
	};
	template<class T>
	using GetLongType = typename GetLongTypeImpl<typename std::decay<T>::type>::type;
}

#define getT(i, t) std::tuple_element_t<i, std::tuple<t...>>
template <template<typename ...> class T, typename ...Types>
struct applyTemplates {
	using type = T<Types...>;
};

template <class lT, template <typename ...> class rT>
struct transTypes_s;
template <template<typename ...> class lT, typename ...T, template<typename ...> class rT>
struct transTypes_s<lT<T...>, rT> {
	using type = rT<T...>;
};

// static_assert(std::is_same<transTypes<std::tuple<int, float>, std::unordered_map>, std::unordered_map<int, float>>::value);
template <class lT, template <typename ...> class rT>
using transTypes = typename transTypes_s<lT, rT>::type;

template <class ...Types>
struct record_types {};

template <class ...Types>
using record = std::tuple<Types...>;

template <class T>
struct decayS {
	using type = typename std::decay<T>::type;
};
template<template<typename ...> class T, typename ...Types>
struct decayS <T<Types...>>{
	using type = T<typename std::decay<Types>::type ...>;
};
template <class T>
using decays = typename decayS<typename std::decay<T>::type>::type;
template <class T>
using decay_inner = typename decayS<T>::type;

template <class, template <class...> class T>
struct instance_of_impl : std::false_type {};
template <class ...T1, template <class ...> class T2>
struct instance_of_impl<T2<T1...>, T2> : std::true_type {};

template <class T1, class T2>
struct same_class_impl : std::false_type {};
template <class ...T1s, class ...T2s, template <class...> class T1>
struct same_class_impl<T1<T1s...>, T1<T2s...>> : std::true_type {};

template <class T1, class T2>
bool same_class = same_class_impl<T1, T2>::value;
template <class T1, template <class...> class T2>
bool instance_of = instance_of_impl<T1, T2>::value;

template <class lT, template <typename ...> class rT>
using transTypes = typename transTypes_s<lT, rT>::type;

template <class lT, class vT, template <vT ...> class rT>
struct transValues_s;
template <class vT, template<class, vT ...> class lT, vT ...T, template<vT ...> class rT>
struct transValues_s<lT<vT, T...>, vT, rT> {
	using type = rT<T...>;
};

#include <utility>
template <class vT, int i, template <vT ...> class rT>
using transValues = typename transValues_s<std::make_integer_sequence<vT, i>, vT, rT>::type;
template <int i, template <int ...> class rT>
using applyIntegerSequence = typename transValues_s<std::make_integer_sequence<int, i>, int, rT>::type;
template <template <class ...> class T, class ...Types>
struct decayed_impl{ typedef T<Types...> type;};
template <template <typename ...> class VT, class ...Types>
using decayed_t = typename decayed_impl<VT, Types...>::type;

template <class First = void, class...Rest>
struct get_first_impl {
	typedef First first;
	constexpr static size_t rest_len = sizeof...(Rest);
	typedef get_first_impl<Rest...> rest;
};
template <class ...T>
using get_first = typename get_first_impl<T...>::first;
template <class T>
struct value_type_impl { typedef T type; };
template <template <class...> class VT, class ...V>
struct value_type_impl<VT<V...>> { typedef get_first<V...> type; };
template <class T>
using value_type = typename value_type_impl<T>::type;
template <class ...T>
using get_first = typename get_first_impl<T...>::first;
template <class T>
struct value_type_rec_impl { typedef T type; };
template <template <class...> class VT, class ...V>
struct value_type_rec_impl<VT<V...>> { typedef typename value_type_rec_impl<get_first<int>>::type type; };
template <class T>
using value_type_r = typename value_type_rec_impl<T>::type;

template <class T>
struct nullval_impl { constexpr static T value = 0; };
template <class T>
constexpr static T nullval = nullval_impl<T>::value;
#include <limits>
template <>
struct nullval_impl<int> { constexpr static int value = std::numeric_limits<int>::min(); };
template<>
struct nullval_impl<float> { constexpr static float value = -std::numeric_limits<float>::quiet_NaN(); };
template<>
struct nullval_impl<double> { constexpr static double value = -std::numeric_limits<double>::quiet_NaN(); };

constexpr size_t sum_type(size_t a[], size_t sz) {
    size_t ret = 0;
    for (int i = 0; i < sz; ++i) 
        ret += a[i];
    return ret;
}
template<class Types, class ...T1> 
constexpr size_t sum_type() {
    size_t t[] = {std::is_same_v<Types, T1> ...};
    return sum_type(t, sizeof...(T1));
}
template<class ...T1, class ...Types> 
constexpr size_t count_type(std::tuple<Types...>* ts) {
    size_t t[] = {sum_type<Types, T1...>() ...};
    return sum_type(t, sizeof...(Types));
}
#endif // !_TYPES_H
