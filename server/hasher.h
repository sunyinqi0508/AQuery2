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

namespace std{
	template<>
	struct hash<astring_view> {
		size_t operator()(const astring_view& _Keyval) const noexcept {
			return append_bytes(_Keyval.str);
		}
	};
	template<>
	struct hash<types::date_t> {
		size_t operator() (const types::date_t& _Keyval) const noexcept {
			return std::hash<unsigned int>()(*(unsigned int*)(&_Keyval));
		}
	};
	template<>
	struct hash<types::time_t> {
		size_t operator() (const types::time_t& _Keyval) const noexcept {
			return std::hash<unsigned int>()(_Keyval.ms) ^ 
			std::hash<unsigned char>()(_Keyval.seconds) ^
			std::hash<unsigned char>()(_Keyval.minutes) ^
			std::hash<unsigned char>()(_Keyval.hours)
			;
		}
	};
	template<>
	struct hash<types::timestamp_t>{
		size_t operator() (const types::timestamp_t& _Keyval) const noexcept {
			return std::hash<types::date_t>()(_Keyval.date) ^ 
				std::hash<types::time_t>()(_Keyval.time);
		}
	};

}

inline size_t append_bytes(const astring_view& view) noexcept {
	return append_bytes(view.str);
}


template <class ...Types>
struct hasher {
	template <size_t i = 0> typename std::enable_if< i == sizeof...(Types), 
		size_t>::type hashi(const std::tuple<Types...>&) const {
		return 534235245539ULL;
	}

	template <size_t i = 0> typename std::enable_if< i < sizeof ...(Types), 
		size_t>::type hashi(const std::tuple<Types...>& record) const {
		using current_type = typename std::decay<typename std::tuple_element<i, std::tuple<Types...>>::type>::type;
		
		return std::hash<current_type>()(std::get<i>(record)) ^ hashi<i+1>(record);
	}
	size_t operator()(const std::tuple<Types...>& record) const {
		return hashi(record);
	}
};
