#pragma once

#include <type_traits>
#include <tuple>
#include "types.h"
// only works for 64 bit systems
constexpr size_t _FNV_offset_basis = 14695981039346656037ULL;
constexpr size_t _FNV_prime = 1099511628211ULL;

inline size_t append_bytes(const unsigned char* _First) noexcept {
	size_t _Val = _FNV_offset_basis;
	for (; *_First; ++_First) {
		_Val ^= static_cast<size_t>(*_First);
		_Val *= _FNV_prime;
	}

	return _Val;
}

inline size_t append_bytes(const astring_view& view) noexcept {
	return append_bytes(view.str);
}


template <class ...Types>
struct hasher {
	template <size_t i = 0> typename std::enable_if< i == sizeof...(Types), 
		size_t>::type hashi(const std::tuple<Types...>& record) const {
		return 534235245539ULL;
	}

	template <size_t i = 0> typename std::enable_if< i < sizeof ...(Types), 
		size_t>::type hashi(const std::tuple<Types...>& record) const {
		using current_type = typename std::decay<typename std::tuple_element<i, std::tuple<Types...>>::type>::type;
		if constexpr (is_cstr<current_type>())
			return append_bytes((const unsigned char*)std::get<i>(record)) ^ hashi<i + 1>(record);
		else
			return std::hash<current_type>()(std::get<i>(record)) ^ hashi<i+1>(record);
	}
	size_t operator()(const std::tuple<Types...>& record) const {
		return hashi(record);
	}
};
