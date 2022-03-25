#ifndef _TABLE_H
#define _TABLE_H

#include "types.h"
#include "vector_type.hpp"

template <typename T>
class vector_type;
template <>
class vector_type<void>;

#ifdef _MSC_VER
namespace types {
	enum Type_t;
	template <typename T>
	struct Types;
	template <class T1, class T2>
	struct Coercion;
}
#endif

template<typename _Ty>
class ColRef : public vector_type<_Ty>
{
public:
	const char* name;
	types::Type_t ty = types::ERROR;
	ColRef(uint32_t size, const char* name = "") : vector_type<_Ty>(size), name(name) {}
	ColRef(const char* name) : name(name) {}
	void init() { ty = types::Types<_Ty>::getType();  this->size = this->capacity = 0; this->container = 0; }
	ColRef(const char* name, types::Type_t ty) : name(name), ty(ty) {}

	template<typename T>
	ColRef<T> scast();
};


template<typename _Ty>
template<typename T>
inline ColRef<T> ColRef<_Ty>::scast()
{
	this->ty = types::Types<T>::getType();
	return *(ColRef<T> *)this;
}
using uColRef = ColRef<void>;
template<class ...Types>
struct TableInfo {
	const char* name;
	ColRef<void>* colrefs;
	uint32_t n_cols, n_rows;
	//void print();
	TableInfo(const char* name, uint32_t n_cols);
};
template <class T>
constexpr static inline bool is_vector(const ColRef<T>&) {
	return true;
}
template <class T>
constexpr static inline bool is_vector(const vector_type<T>&) {
	return true;
}
template <class T>
constexpr static inline bool is_vector(const T&) {
	return false;
}
template<class ...Types>
TableInfo<Types...>::TableInfo(const char* name, uint32_t n_cols) : name(name), n_cols(n_cols) {
	this->colrefs = (ColRef<void>*)malloc(sizeof(ColRef<void>) * n_cols);
}

template <class T1, class T2>
ColRef<typename types::Coercion<T1, T2>::type> operator -(const ColRef<T1>& lhs, const ColRef<T2>& rhs) {
	auto ret = ColRef<typename types::Coercion<T1, T2>::type>(lhs.size, "");
	for (int i = 0; i < lhs.size; ++i)
		ret.container[i] = lhs.container[i] - rhs.container[i];
	return ret;
}
template <class T1, class T2>
ColRef<typename types::Coercion<T1, T2>::type> operator -(const ColRef<T1>& lhs, const T2& rhs) {
	auto ret = ColRef<typename types::Coercion<T1, T2>::type>(lhs.size, "");
	for (int i = 0; i < lhs.size; ++i)
		ret.container[i] = lhs.container[i] - rhs;
	return ret;
}

template <size_t _Index, class... _Types>
constexpr ColRef<std::tuple_element_t<_Index, std::tuple<_Types...>>>& get(TableInfo<_Types...>& table) noexcept {
	return *(ColRef< std::tuple_element_t<_Index, std::tuple<_Types...>>> *) &(table.colrefs[_Index]);
}


//void TableInfo::print()
//{
//	for (int i = 0; i < n_rows; ++i) {
//		for (int j = 0; j < n_cols; ++j) // TODO: Deal with date/time
//			printf(types::printf_str[colrefs[j].ty], colrefs[j].get(i, colrefs[j].ty));
//		puts("");
//	}
//}
template<class T>
ColRef<T> mins(const ColRef<T>& arr) {
	const int& len = arr.size;
	std::deque<std::pair<T, uint32_t>> cache;
	ColRef<T> ret(len);
	T min = std::numeric_limits<T>::max();
	for (int i = 0; i < len; ++i) {
		if (arr[i] < min)
			min = arr[i];
		ret[i] = min;
	}
	return ret;
}
template<class T>
ColRef<T> maxs(const ColRef<T>& arr) {
	const int& len = arr.size;
	ColRef<T> ret(len);
	T max = std::numeric_limits<T>::min();
	for (int i = 0; i < len; ++i) {
		if (arr[i] > max)
			max = arr[i];
		ret[i] = max;
	}
	return ret;
}

template<class T>
ColRef<T> minw(const ColRef<T>& arr, uint32_t w) {
	const int& len = arr.size;
	ColRef<T> ret(len);
	std::deque<std::pair<T, uint32_t>> cache;
	for (int i = 0; i < len; ++i) {
		if (!cache.empty() && cache.front().second == i - w) cache.pop_front();
		while (!cache.empty() && cache.back().first > arr[i]) cache.pop_back();
		cache.push_back({ arr[i], i });
		ret[i] = cache.front().first;
	}
	return ret;
}

template<class T>
ColRef<T> maxw(const ColRef<T>& arr, uint32_t w) {
	const int& len = arr.size;
	ColRef<T> ret(len);
	std::deque<std::pair<T, uint32_t>> cache;
	for (int i = 0; i < len; ++i) {
		if (!cache.empty() && cache.front().second == i - w) cache.pop_front();
		while (!cache.empty() && cache.back().first > arr[i]) cache.pop_back();
		cache.push_back({ arr[i], i });
		arr[i] = cache.front().first;
	}
	return ret;
}

template <class T>
void print(const T& v) {
	printf(types::printf_str[types::Types<T>::getType()], v);
}

template <class T>
void print(const ColRef<T>& v, const char* delimiter) {

	for (const auto& vi : v) {
		print(vi);
		puts(delimiter);
	}
}
#endif