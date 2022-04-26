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
	static constexpr const char* printf_str[] = { "%d", "%f", "%s", "%lf", "%llf", "%ld", "%hi", "%s", "%s", "%c",
		"%u", "%lu", "%hu", "%hhu", "NULL" };
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
	struct GetFPTypeImpl {
		using type = Cond(__Eq(float), float, Cond(__Eq(double), double, long double));
	};
	template<class T>
	using GetFPType = typename GetFPTypeImpl<typename std::decay<T>::type>::type;
	template<class T>
	struct GetLongTypeImpl {
		using type = Cond(_U(T), unsigned long long, Cond(Fp(T), long double, long long));
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
#endif // !_TYPES_H
