#ifndef _TABLE_H
#define _TABLE_H

#include "types.h"
#include "vector_type.hpp"
#include <iostream>
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
class ColView;
template<typename _Ty>
class ColRef : public vector_type<_Ty>
{
public:
	const char* name;
	types::Type_t ty = types::ERROR;
	ColRef(const uint32_t& size, const char* name = "") : vector_type<_Ty>(size), name(name) {}
	ColRef(const char* name) : name(name) {}
	void init() { ty = types::Types<_Ty>::getType();  this->size = this->capacity = 0; this->container = 0; }
	ColRef(const char* name, types::Type_t ty) : name(name), ty(ty) {}
	using vector_type<_Ty>::operator[];
	using vector_type<_Ty>::operator=;
	ColView<_Ty> operator [](const vector_type<uint32_t>&idxs) const {
		return ColView<_Ty>(*this, idxs);
	}
	template<typename T>
	ColRef<T> scast();
};

template<typename _Ty>
class ColView {
public: 
	const vector_type<uint32_t>& idxs;
	const ColRef<_Ty>& orig;
	const uint32_t& size;
	ColView(const ColRef<_Ty>& orig, const vector_type<uint32_t>& idxs) : orig(orig), idxs(idxs), size(idxs.size) {}
	ColView(const ColView<_Ty>& orig, const vector_type<uint32_t>& idxs) : orig(orig.orig), idxs(idxs), size(idxs.size) {
		for (uint32_t i = 0; i < size; ++i) 
			idxs[i] = orig.idxs[idxs[i]];
	}
	_Ty& operator [](const uint32_t& i) const {
		return orig[idxs[i]];
	}
	struct Iterator_t {
		const uint32_t* val;
		const ColRef<_Ty>& orig;
		constexpr Iterator_t(const uint32_t* val, const ColRef<_Ty>& orig) noexcept : val(val), orig(orig) {}
		_Ty& operator*() { return orig[*val]; }
		bool operator != (const Iterator_t& rhs) { return rhs.val != val; }
		Iterator_t& operator++ () {
			++val;
			return *this;
		}
		Iterator_t operator++ (int) {
			Iterator_t tmp = *this;
			++val;
			return tmp;
		}
	};
	Iterator_t begin() const {
		return Iterator_t(idxs.begin(), orig);
	}
	Iterator_t end() const {
		return Iterator_t(idxs.end(), orig);
	}
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
	uint32_t n_cols;
	void print(const char* __restrict sep, const char* __restrict end) const;
	typedef std::tuple<Types...> tuple_type;
	template <size_t j = 0>
	typename std::enable_if<j == sizeof...(Types) - 1, void>::type print_impl(const uint32_t& i, const char* __restrict sep = " ") const;
	template <size_t j = 0>
	typename std::enable_if<j < sizeof...(Types) - 1, void>::type print_impl(const uint32_t& i, const char* __restrict sep = " ") const;
	template <size_t ...Idxs>
	struct GetTypes {
		typedef typename std::tuple<typename std::tuple_element<Idxs, tuple_type>::type ...> type;
	};
	template <size_t ...Idxs>
	using getRecordType = typename GetTypes<Idxs...>::type;
	TableInfo(const char* name, uint32_t n_cols);
	//template<size_t ...Idxs = Types...>
	//struct Iterator_t {
	//	uint32_t val;
	//	const TableInfo* info;
	//	constexpr Iterator_t(const uint32_t* val, const TableInfo* info) noexcept : val(val), info(info) {}
	//	getRecordType<Idxs...> operator*() {
	//		return getRecordType<Idxs...>(info->colrefs[Idxs].operator[](*val)...);
	//	}
	//	bool operator != (const Iterator_t& rhs) { return rhs.val != val; }
	//	Iterator_t& operator++ () {
	//		++val;
	//		return *this;
	//	}
	//	Iterator_t operator++ (int) {
	//		Iterator_t tmp = *this;
	//		++val;
	//		return tmp;
	//	}
	//};
	//template<size_t ...Idxs = Types...>
	//Iterator_t<Idxs...> begin() const {
	//	
	//}
	//template<size_t ...Idxs = Types...>
	//Iterator_t<Idxs...> end() const {
	//	
	//}
	//
	//template<int ...Cols, bool ...ord>
	//order_by() {
	//	vector_type<uint32_t> order(colrefs[0].size);
	//	std::sort(this->begin<Cols>)
	//}
};

template <size_t _Index, class... _Types>
constexpr inline ColRef<std::tuple_element_t<_Index, std::tuple<_Types...>>>& get(const TableInfo<_Types...>& table) noexcept {
	return *(ColRef<std::tuple_element_t<_Index, std::tuple<_Types...>>> *) & (table.colrefs[_Index]);
}

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

template <class ...Types>
template <size_t j>
inline typename std::enable_if<j == sizeof...(Types) - 1, void>::type
	TableInfo<Types ...>::print_impl(const uint32_t& i, const char* __restrict sep) const {
	std::cout << (get<j>(*this))[i];
}

template<class ...Types>
template<size_t j>
inline typename std::enable_if<j < sizeof...(Types) - 1, void>::type 
	TableInfo<Types...>::print_impl(const uint32_t& i, const char* __restrict sep) const
{
	std::cout << (get<j>(*this))[i] << sep;
	print_impl<j+1>(i, sep);
}

template<class ...Types>
inline void TableInfo<Types...>::print(const char* __restrict sep, const char* __restrict end) const {
	int n_rows = 0;
	if (n_cols > 0 && colrefs[0].size > 0)
		n_rows = colrefs[0].size;
	for (int i = 0; i < n_rows; ++i) {
		print_impl(i);
		std::cout << end;
	}
}
template <class T1, class T2, template<typename ...> class VT>
VT<typename types::Coercion<T1, T2>::type> operator -(const VT<T1>& lhs, const VT<T2>& rhs) {
	auto ret = VT<typename types::Coercion<T1, T2>::type>(lhs.size, "");
	for (int i = 0; i < lhs.size; ++i)
		ret.container[i] = lhs.container[i] - rhs.container[i];
	return ret;
}
template <class T1, class T2, template<typename ...> class VT>
VT<typename types::Coercion<T1, T2>::type> operator -(const VT<T1>& lhs, const T2& rhs) {
	auto ret = VT<typename types::Coercion<T1, T2>::type>(lhs.size, "");
	for (int i = 0; i < lhs.size; ++i)
		ret.container[i] = lhs.container[i] - rhs;
	return ret;
}
template <class T1, class T2, template<typename ...> class VT>
VT<typename types::Coercion<T1, T2>::type> operator +(const VT<T1>& lhs, const VT<T2>& rhs) {
	auto ret = VT<typename types::Coercion<T1, T2>::type>(lhs.size, "");
	for (int i = 0; i < lhs.size; ++i)
		ret.container[i] = lhs.container[i] + rhs.container[i];
	return ret;
}
template <class T1, class T2, template<typename ...> class VT>
VT<typename types::Coercion<T1, T2>::type> operator +(const VT<T1>& lhs, const T2& rhs) {
	auto ret = VT<typename types::Coercion<T1, T2>::type>(lhs.size, "");
	for (int i = 0; i < lhs.size; ++i)
		ret.container[i] = lhs.container[i] + rhs;
	return ret;
}
template <class T1, class T2, template<typename ...> class VT>
VT<typename types::Coercion<T1, T2>::type> operator *(const VT<T1>& lhs, const VT<T2>& rhs) {
	auto ret = VT<typename types::Coercion<T1, T2>::type>(lhs.size, "");
	for (int i = 0; i < lhs.size; ++i)
		ret.container[i] = lhs.container[i] * rhs.container[i];
	return ret;
}
template <class T1, class T2, template<typename ...> class VT>
VT<typename types::Coercion<T1, T2>::type> operator *(const VT<T1>& lhs, const T2& rhs) {
	auto ret = VT<typename types::Coercion<T1, T2>::type>(lhs.size, "");
	for (int i = 0; i < lhs.size; ++i)
		ret.container[i] = lhs.container[i] * rhs;
	return ret;
}
template <class T1, class T2, template<typename ...> class VT>
VT<typename types::Coercion<T1, T2>::type> operator /(const VT<T1>& lhs, const VT<T2>& rhs) {
	auto ret = VT<typename types::Coercion<T1, T2>::type>(lhs.size, "");
	for (int i = 0; i < lhs.size; ++i)
		ret.container[i] = lhs.container[i] / rhs.container[i];
	return ret;
}
template <class T1, class T2, template<typename ...> class VT>
VT<typename types::Coercion<T1, T2>::type> operator /(const VT<T1>& lhs, const T2& rhs) {
	auto ret = VT<typename types::Coercion<T1, T2>::type>(lhs.size, "");
	for (int i = 0; i < lhs.size; ++i)
		ret.container[i] = lhs.container[i] / rhs;
	return ret;
}

template <class ...Types>
void print(const TableInfo<Types...>& v, const char* delimiter = " ", const char* endline = "\n") {
	v.print(delimiter, endline);
}
template <class T>
void print(const T& v, const char* delimiter = " ") {
	printf(types::printf_str[types::Types<T>::getType()], v);
}
template <class T>
void inline print_impl(const T& v, const char* delimiter, const char* endline) {
	for (const auto& vi : v) {
		print(vi);
		printf("%s", delimiter);
	}
	printf("%s", endline);
}

template <class T, template<typename> class VT>
typename std::enable_if<!std::is_same<VT<T>, TableInfo<T>>::value>::type 
print(const VT<T>& v, const char* delimiter = " ", const char* endline = "\n") {
	print_impl(v, delimiter, endline);
}
 
#endif