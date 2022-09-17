#pragma once
#include "vector_type.hpp"
#include <algorithm>
#include <stdint.h>
template <class Comparator, typename T = uint32_t>
class priority_vector : public vector_type<T> {
	const Comparator comp;
public:
	priority_vector(Comparator comp = std::less<T>{}) : 
		comp(comp), vector_type<T>(0) {}
	void emplace_back(T val) {
		vector_type<T>::emplace_back(val);
		std::push_heap(container, container + size, comp);
	}
	void pop_back() {
		std::pop_heap(container, container + size, comp);
		--size;
	}
}; 