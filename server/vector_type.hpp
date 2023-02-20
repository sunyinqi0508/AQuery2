/*
* Bill Sun 2022
*/


#ifndef _VECTOR_TYPE
#define _VECTOR_TYPE

#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <iterator>
#include <initializer_list>
#include <unordered_set>
#include <iostream>
#include "hasher.h"
#include "types.h"
#include "gc.h"
#pragma pack(push, 1)

struct vectortype_cstorage {
	void* container;
	unsigned int size, capacity;
};

template <typename _Ty>
class vector_type : public vector_base<_Ty>{
public:
	typedef vector_type<_Ty> Decayed_t;
	void inline _copy(const vector_type<_Ty>& vt) {
		// quick init while using malloc
		//if (capacity > 0) free(container);
		this->size = vt.size;
		this->capacity = vt.capacity;
		if (capacity) {
			//puts("copy");
			this->container = (_Ty*)malloc(size * sizeof(_Ty));
			memcpy(container, vt.container, sizeof(_Ty) * size);
		}
		else {
			this->container = vt.container;
		}
	}
	void inline _move(vector_type<_Ty>&& vt) {
		if (capacity > 0) free(container);
		
		this->size = vt.size;
		this->capacity = vt.capacity;
		this->container = vt.container;
		// puts("move");
		vt.size = vt.capacity = 0;	
		vt.container = nullptr;
	}
public:
	_Ty* container;
	uint32_t size, capacity;
	typedef _Ty* iterator_t;
	typedef std::conditional_t<is_cstr<_Ty>(), astring_view, _Ty> value_t;
	explicit vector_type(const uint32_t& size) : size(size), capacity(size) {
		if (GC::scratch_space != nullptr) {
			[[likely]]
			container = (_Ty*)GC::scratch_space->alloc(size * sizeof(_Ty));
			this->capacity = 0;
		}
		else{
			container = (_Ty*)malloc(size * sizeof(_Ty));
		}
		// TODO: calloc for objects. 
	}
	explicit constexpr vector_type(std::initializer_list<_Ty> _l) {
		size = capacity = _l.size();
		this->container = (_Ty*)malloc(sizeof(_Ty) * capacity);
		_Ty* _container = this->container;
		for (const auto& l : _l) {
			*(_container++) = l;
		}
	}
	constexpr vector_type() noexcept : size(0), capacity(0), container(0) {};
	constexpr vector_type(_Ty* container, uint32_t len) noexcept : size(len), capacity(0), container(container) {};
	vector_type(const char** container, uint32_t len, 
		typename std::enable_if_t<!std::is_same_v<_Ty, const char*>>* = nullptr) noexcept = delete;
	constexpr explicit vector_type(const vector_type<_Ty>& vt) noexcept : capacity(0) {
		_copy(vt);
	}
	constexpr vector_type(vector_type<_Ty>& vt) noexcept : capacity(0) {
		_move(std::move(vt));
	}
	constexpr vector_type(vector_type<_Ty>&& vt) noexcept : capacity(0) {
		_move(std::move(vt));
	}
	explicit vector_type(vectortype_cstorage vt) noexcept : 
		capacity(vt.capacity), size(vt.size), container((_Ty*)vt.container) {
		// out(10);
	};
	// size >= capacity ==> readonly vector
	vector_type(const uint32_t size, void* data) : 
		size(size), capacity(0), container(static_cast<_Ty*>(data)) {}

	void init_from(const uint32_t size, void* data) {
		this->container = static_cast<_Ty*>(data);
		this->size = size;
		this->capacity = 0;
	}
	
	vector_type<_Ty>& operator =(const _Ty& vt) {
		if (!container) { 
			container = (_Ty*)malloc(sizeof(_Ty)); 
			capacity = 1; 
		}
		size = 1;
		container[0] = vt;
		return *this;
	}
	// template<template <typename ...> class VT, class T>
	// vector_type<_Ty>& operator =(VT<T>&& vt) {
	// 	this->container = (_Ty*)vt.container;
	// 	this->size = vt.size;
	// 	this->capacity = vt.capacity;
	// 	vt.capacity = 0; // rvalue's 
	// 	return *this;
	// }
	vector_type<_Ty>& operator =(const vector_type<_Ty>& vt) {
		_copy(vt);
		return *this;
	}
	vector_type<_Ty>& operator =(vector_type<_Ty>&& vt) noexcept {
		_move(std::move(vt));
		return *this;
	}
	template <template <class> class VT>
	vector_type<_Ty>& operator =(const VT<_Ty>& vt) {
		if (capacity > 0) free(container);
		container = static_cast<_Ty*>(malloc(vt.size * sizeof(_Ty)));
		
		size = vt.size;
		capacity = size;
		for(uint32_t i = 0; i < size; ++i)
			container[i] = vt[i];
		
		return *this;
	}
	inline std::unordered_set<value_t> distinct_common() {
		return std::unordered_set<value_t>(container, container + size);
	}
	vector_type<_Ty>& distinct_inplace() {
		uint32_t i = 0;
		for(const auto& v : distinct_common()){
			container[i++] = v;
		}
		return *this;
	} 
	vector_type<_Ty> distinct_copy(){
		auto d_vals = distinct_common();
		vector_type<_Ty> ret(d_vals.size());
		uint32_t i = 0;
		for(const auto& v : d_vals){
			ret[i++] = v;
		}
		return ret;
	}
	uint32_t distinct_size(){
		return distinct_common().size();
	}
	vector_type<_Ty> distinct(){
		if (capacity)
			return distinct_inplace();
		else
			return distinct_copy();
	}
	// TODO: think of situations where this is a temp!! (copy on write!!!)
	template <bool _grow = true>
	inline void grow(uint32_t sz = 0) {
		if constexpr (_grow)
			sz = this->size;
		if (sz >= capacity) { // geometric growth
			uint32_t new_capacity;
			if constexpr (_grow)
				new_capacity = size + 1 + (size >> 1);
			else	
				new_capacity = sz;

			_Ty* n_container = (_Ty*)realloc(container, new_capacity * sizeof(_Ty));
			// memcpy(n_container, container, sizeof(_Ty) * size);
			memset(n_container + size, 0, sizeof(_Ty) * (new_capacity - size));
			// if (capacity)
			// 	free(container);
			container = n_container;
			capacity = new_capacity;
		}
	}
	inline void resize(const uint32_t sz){
		size = sz;
		grow<false>(sz);
	}
	inline void reserve(const uint32_t sz){
		grow<false>(sz);
	}

	inline void emplace_back(const _Ty& _val) {
		grow();
		container[size++] = _val;
	}
	inline void emplace_back(_Ty& _val) {
		grow();
		container[size++] = std::move(_val);
	}
	inline void emplace_back(_Ty&& _val) {
		grow();
		container[size++] = std::move(_val);
	}
	iterator_t erase(iterator_t _it) {
#ifdef DEBUG 
		// Do bound checks 
		if (!(size && capicity && container &&
			_it >= container && (_it - container) < size))
			return 0;
#endif
		iterator_t curr = _it + 1, end = _it + --size;
		while (curr < end)
			*(curr - 1) = *(curr++);
		return _it;
	}

	inline iterator_t begin() const {
		return container;
	}
	inline iterator_t end() const {
		return container + size;
	}

	iterator_t find(const _Ty item) const {
		iterator_t curr = begin(), _end = end();
		while (curr != _end && *(curr++) != item);
		return curr;
	}

	inline _Ty& operator[](const uint32_t _i) const {
		return container[_i];
	}

	inline void shrink_to_fit() {
		if (size && capacity != size) {
			capacity = size;
			_Ty* _container = (_Ty*)malloc(sizeof(_Ty) * size);
			memcpy(_container, container, sizeof(_Ty) * size);
			free(container);
			container = _container;
		}
	}

	inline void clear() {
		this->size = 0;
	}

	inline _Ty& back() {
		return container[size - 1];
	}
	inline void qpop() {
		size = size ? size - 1 : size;
	}
	inline void pop_resize() {
		if (size) {
			--size;
			if (capacity > (size << 1))
			{
				_Ty* new_container = (_Ty*)malloc(sizeof(_Ty) * size);
				memcpy(new_container, container, sizeof(_Ty) * size);
				free(container);
				container = new_container;
				capacity = size;
			}
		}
	}
	inline _Ty pop() {
		return container[--size];
	}
	void merge(vector_type<_Ty>& _other) {
		if (capacity < this->size + _other.size)
		{
			_Ty* new_container = (_Ty*)malloc(sizeof(_Ty) * (_other.size + this->size));
			capacity = this->size + _other.size;
			memcpy(new_container, container, sizeof(_Ty) * this->size);
			memcpy(new_container + this->size, _other.container, sizeof(_Ty) * _other.size);
			free(container);
			container = new_container;
		}
		else
			memcpy(container + this->size, _other.container, sizeof(_Ty) * _other.size);
		size = this->size + _other.size;
	}
	template<class _Iter>
	void merge(_Iter begin, _Iter end) {
		unsigned int dist = static_cast<unsigned int>(std::distance(begin, end));
		if (capacity < this->size + dist) {
			_Ty* new_container = (_Ty*)malloc(sizeof(_Ty) * (dist + this->size));
			capacity = this->size + dist;
			memcpy(new_container, container, sizeof(_Ty) * this->size);
			free(container);
			container = new_container;
		}

		for (int i = 0; i < dist; ++i) {
			container[i + this->size] = *(begin + i);
		}
		size = this->size + dist;
	}
	inline void out(uint32_t n = 4000, const char* sep = " ") const
	{
		const char* more = "";
		if (n < this->size)
			more = " ... ";
		else
			n = this->size;

		std::cout << '(';
		if (n > 0)
		{
			uint32_t i = 0;
			for (; i < n - 1; ++i)
				std::cout << this->operator[](i) << sep;
			std::cout << this->operator[](i);
		}
		std::cout<< more;
		std::cout << ')';
	}
	vector_type<_Ty> subvec_memcpy(uint32_t start, uint32_t end) const {
		vector_type<_Ty> subvec(end - start);
		memcpy(subvec.container, container + start, sizeof(_Ty) * (end - start));
		return subvec;
	}
	inline vector_type<_Ty> subvec(uint32_t start, uint32_t end) const {
		return vector_type<_Ty>(container + start, end - start);
	}
	vector_type<_Ty> subvec_deep(uint32_t start, uint32_t end) const {
		uint32_t len = end - start;
		vector_type<_Ty> subvec(len);
		for (uint32_t i = 0; i < len; ++i)
			subvec[i] = container[i];
		return subvec;
	}
	inline vector_type<_Ty> subvec(uint32_t start = 0) { return subvec(start, size); }
	inline vector_type<_Ty> subvec_memcpy(uint32_t start = 0) const { return subvec_memcpy(start, size); }
	inline vector_type<_Ty> subvec_deep(uint32_t start = 0) const { return subvec_deep(start, size); }
	vector_type<_Ty> getRef() { return vector_type<_Ty>(container, size); }
	~vector_type() {
		if (capacity > 0) GC::gc_handle->reg(container, sizeof(_Ty) * capacity);//free(container);
		container = nullptr; 
		size = capacity = 0;
	}
#define Compare(_op) \
	template<typename T> \
	inline vector_type<bool> operator _op (const T& v){ \
		vector_type<bool> res(size); \
		for (uint32_t i = 0; i < size; ++i) \
			res[i] = container[i] > v; \
		return res; \
	} 
	// template <typename T> 
	Compare(>)
	Compare(<)
	Compare(>=)
	Compare(<=)
	Compare(==)
	Compare(!=)
#define Op(o, x) \
	template<typename T>\
	vector_type<typename types::Coercion<_Ty, T>::type> inline x(const vector_type<T>& r) const {\
		vector_type<typename types::Coercion<_Ty, T>::type> ret(size);\
		for (int i = 0; i < size; ++i)\
			ret[i] = container[i] o r[i];\
		return ret;\
	}

#define Opeq(o, x) \
	template<typename T>\
	inline vector_type<typename types::Coercion<_Ty, T>::type>& x##eq(const vector_type<T>& r) {\
		for (int i = 0; i < size; ++i)\
			container[i] = container[i] o##= r[i];\
		return *this;\
	}

#define Ops(o, x) \
	template<typename T>\
	vector_type<typename types::Coercion<_Ty, T>::type> operator o (const vector_type<T>& r) const {\
		/*if (r.size == size) { [[likely]] */\
			return x(r);\
		/*}*/\
	}

#define Opseq(o, x) \
	template<typename T>\
	vector_type<typename types::Coercion<_Ty, T>::type> operator o##= (const vector_type<T>& r) {\
		/*if (r.size == size) { [[likely]] */\
			return x##eq(r);\
		/*}*/\
	}

#define _Make_Ops(M) \
	M(+, add) \
	M(-, minus) \
	M(*, multi) \
	M(/, div) \
	M(%, mod) 

	_Make_Ops(Op)
	_Make_Ops(Opeq)
	_Make_Ops(Ops)
	_Make_Ops(Opseq)
};


// template<>
// void vector_type<std::string_view>::init_from(const uint32_t size, void* data) {
// 	this->size = this->capacity = size;
// 	this->container = static_cast<std::string_view*>(
// 		malloc(sizeof(std::string_view) * size));
// 	for(uint32_t i = 0; i < size; ++i){
// 		this->container[i] = container[i];
// 	}
// }

// template<template <typename> class VT>
// inline void 
// prealloc_vector (VT &vt, uint32_t sz) { 
// 	vt.reserve(sz); 
// }

// template<class T>
// inline void 
// prealloc_vector (vector_type<vector_type<T>> &vt, 
// 				uint32_t outer_sz, uint32_t inner_sz) { 
// 	vt.reserve(outer_sz); 
// 	auto mem = static_cast<T*>(malloc(inner_sz * sizeof(T)));
// }

template <>
class vector_type<void> {
public:
	void* container;
	uint32_t size, capacity;
	typedef void* iterator_t;

	vector_type(uint32_t size) : size(size), capacity(size) {
		container = (void*)malloc(size);
	}
	template<typename _Ty>
	constexpr vector_type(std::initializer_list<_Ty> _l) {
		size = capacity = _l.size();
		this->container = malloc(sizeof(_Ty) * _l.size());
		_Ty* _container = (_Ty*)this->container;
		for (const auto& l : _l) {
			*(_container++) = l;
		}
	}
	constexpr vector_type() : size(0), capacity(0), container(0) {};
	void* get(uint32_t i, types::Type_t atype){
		return static_cast<void*>(static_cast<char*>(container) + (i * types::AType_sizes[atype]));
	}
	void operator[](const uint32_t& i);
	vector_type<void> subvec(uint32_t, uint32_t);
	vector_type<void> subvec_memcpy(uint32_t, uint32_t);
	vector_type<void> subvec_deep(uint32_t, uint32_t);
	vector_type<void> subvec(uint32_t);
	vector_type<void> subvec_memcpy(uint32_t);
	vector_type<void> subvec_deep(uint32_t);
};
#pragma pack(pop)

template <class Key, class Hash>
class AQHashTable : public ankerl::unordered_dense::set<Key, Hash> {
public:
	uint32_t* reversemap, *mapbase, *ht_base;
	AQHashTable() = default;
	explicit AQHashTable(uint32_t sz) 
		: ankerl::unordered_dense::set<Key, Hash>{} {
		this->reserve(sz);
		this->m_values.reserve(sz);
		reversemap = static_cast<uint32_t *>(malloc(sizeof(uint32_t) * sz * 2));
		mapbase = reversemap + sz;
		ht_base =  static_cast<uint32_t *>(calloc(sz, sizeof(uint32_t)));
	}

	void init(uint32_t sz) {
		ankerl::unordered_dense::set<Key, Hash>::reserve(sz);
		reversemap = static_cast<uint32_t *>(malloc(sizeof(uint32_t) * sz * 2));
		mapbase = reversemap + sz;
		ht_base =  static_cast<uint32_t *>(calloc(sz, sizeof(uint32_t)));
	}

	inline void hashtable_push(Key&& k, uint32_t i){
		reversemap[i] = ankerl::unordered_dense::set<Key, Hash>::hashtable_push(std::move(k));
		++ht_base[reversemap[i]];
	}

	auto ht_postproc(uint32_t sz) {
		auto& arr_values = this->values();
		const auto& len = this->size();

		auto vecs = static_cast<vector_type<uint32_t>*>(malloc(sizeof(vector_type<uint32_t>) * len));
		vecs[0].init_from(ht_base[0], mapbase);
		for (uint32_t i = 1; i < len; ++i) {
			vecs[i].init_from(ht_base[i], mapbase + ht_base[i - 1]);
			ht_base[i] += ht_base[i - 1];
		}
		for (uint32_t i = 0; i < sz; ++i) {
			auto id = reversemap[i];
			mapbase[--ht_base[id]] = i;    
		}
		return vecs;
	}
};

template<>
vector_type<std::string_view>::vector_type(const uint32_t size, void* data);
template <>
vector_type<std::string_view>::vector_type(const char** container, uint32_t len, 
		typename std::enable_if_t<true>*) noexcept;

#endif
