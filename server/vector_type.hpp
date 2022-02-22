/*
* Bill Sun 2022
*/


#ifndef _VECTOR_TYPE
#define _VECTOR_TYPE

#include <cstring>
#include <cstdlib>
#include <cstdint>

#include <algorithm>
#include <initializer_list>
#include <vector>
#include <stdarg.h>

#include "types.h"

#pragma pack(push, 1)
template <typename _Ty>
class vector_type {
public:
	void inline _copy(vector_type<_Ty>& vt) {
		this->size = vt.size;
		this->capacity = vt.capacity;
		this->container = (_Ty*)malloc(size * sizeof(_Ty));

		memcpy(container, vt.container, sizeof(_Ty) * size);
	}
	void inline _move(vector_type<_Ty>&& vt) {
		this->size = vt.size;
		this->capacity = vt.capacity;
		this->container = vt.container;

		vt.size = vt.capacity = 0;	
		vt.container = 0;
	}
public:
	_Ty* container;
	uint32_t size, capacity;
	typedef _Ty* iterator_t;
	vector_type(uint32_t size) : size(size), capacity(size) {
		container = (_Ty*)malloc(size * sizeof(_Ty));
	}
	constexpr vector_type(std::initializer_list<_Ty> _l) {
		size = capacity = _l.size();
		_Ty* container = this->container = (_Ty*)malloc(sizeof(_Ty) * _l.size());
		for (const auto& l : _l) {
			*(container++) = l;
		}
	}
	constexpr vector_type() noexcept = default;
	constexpr vector_type(vector_type<_Ty>& vt) noexcept {
		_copy(vt);
	}
	constexpr vector_type(vector_type<_Ty>&& vt) noexcept {
		_move(std::move(vt));
	}
	vector_type<_Ty> operator =(vector_type<_Ty>& vt) {
		_copy(vt);
		return *this;
	}
	vector_type<_Ty> operator =(vector_type<_Ty>&& vt) {
		_move(std::move(vt));
		return *this;
	}
	void emplace_back(_Ty _val) {
		if (size >= capacity) { // geometric growth
			capacity += 1 + capacity >> 1;
			_Ty* n_container = (_Ty*)malloc(capacity * sizeof(_Ty));
			memcpy(n_container, container, sizeof(_Ty) * size);
			free(container);
			container = n_container;
		}
		container[size++] = _val;
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

	iterator_t begin() const {
		return container;
	}
	iterator_t end() const {
		return container + size;
	}

	iterator_t find(const _Ty item) const {
		iterator_t curr = begin(), _end = end();
		while (curr != _end && *(curr++) != item);
		return curr;
	}

	_Ty& operator[](const std::size_t _i) const {
		return container[_i];
	}

	void shrink_to_fit() {
		if (size && capacity != size) {
			capacity = size;
			_Ty* _container = (_Ty*)malloc(sizeof(_Ty) * size);
			memcpy(_container, container, sizeof(_Ty) * size);
			free(container);
			container = _container;
		}
	}

	_Ty& back() {
		return container[size - 1];
	}
	void qpop() {
		size = size ? size - 1 : size;
	}
	void pop() {
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
	_Ty pop_back() {
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
	~vector_type() {
		if (capacity > 0) free(container);
		container = 0; size = capacity = 0;
	}
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
	vector_type<typename types::Coercion<_Ty, T>::type> inline x##eq(const vector_type<T>& r) {\
		for (int i = 0; i < size; ++i)\
			container[i] = container[i] o##= r[i];\
		return *this;\
	}

#define Ops(o, x) \
	template<typename T>\
	vector_type<typename types::Coercion<_Ty, T>::type> operator o (const vector_type<T>& r) const {\
		[[likely]] if (r.size == size) {\
			return x(r);\
		}\
	}

#define Opseq(o, x) \
	template<typename T>\
	vector_type<typename types::Coercion<_Ty, T>::type> operator o##= (const vector_type<T>& r) {\
		[[likely]] if (r.size == size) {\
			return x##eq(r);\
		}\
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
#pragma pack(pop)
#endif
