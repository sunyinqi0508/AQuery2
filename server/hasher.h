#pragma once

#include <type_traits>
#include <tuple>
#include <functional>
#include <string_view>
#include "types.h"
// #include "robin_hood.h"
#include "unordered_dense.h"
// only works for 64 bit systems
namespace hasher_consts{
	constexpr size_t _FNV_offset_basis = 14695981039346656037ULL;
	constexpr size_t _FNV_prime = 1099511628211ULL;
}

inline size_t append_bytes(const unsigned char* _First) noexcept {
	size_t _Val = hasher_consts::_FNV_offset_basis;
	for (; *_First; ++_First) {
		_Val ^= static_cast<size_t>(*_First);
		_Val *= hasher_consts::_FNV_prime;
	}
	
	return _Val;
}

inline size_t append_bytes(const astring_view& view) noexcept {
	return append_bytes(view.str);
}

#ifdef __SIZEOF_INT128__
union int128_struct
{
	struct {
		uint64_t low, high;
	}__struct;
	__int128_t value = 0;
	__uint128_t uvalue;
	constexpr int128_struct() : value(0) {}
	constexpr int128_struct(const __int128_t &value) noexcept : value(value) {}
	constexpr int128_struct(const __uint128_t &value) noexcept : uvalue(value) {}
	operator __int128_t () const {
		return value;
	}
	operator __uint128_t () const {
		return uvalue;
	}
	operator __int128_t& () {
		return value;
	}
	operator __uint128_t& () {
		return uvalue;
	}
};
#endif
template <class ...Types>
struct hasher {
	template <size_t i = 0> typename std::enable_if< i == sizeof...(Types),
		size_t>::type hashi(const std::tuple<Types...>&) const {
		return 534235245539ULL;
	}

	template <size_t i = 0> typename std::enable_if < i < sizeof ...(Types),
		size_t>::type hashi(const std::tuple<Types...>& record) const {
		using current_type = typename std::decay<typename std::tuple_element<i, std::tuple<Types...>>::type>::type;
#ifdef __SIZEOF_INT128__
		using _current_type = typename std::conditional_t<
			std::is_same_v<current_type, __uint128_t> || 
			std::is_same_v<current_type, __int128_t>,
			int128_struct, current_type>;
#else
		#define _current_type current_type
#endif
		return ankerl::unordered_dense::hash<_current_type>()(std::get<i>(record)) ^ hashi<i + 1>(record);
	}
	size_t operator()(const std::tuple<Types...>& record) const {
		return hashi(record);
	}
};
template <class T>
struct hasher<T>{
	size_t operator()(const std::tuple<T>& record) const {
		return ankerl::unordered_dense::hash<T>()(std::get<0>(record));
	}
};

namespace ankerl::unordered_dense{
	template<>
	struct hash<astring_view> {
		size_t operator()(const astring_view& _Keyval) const noexcept {
			
			return ankerl::unordered_dense::hash<std::string_view>()(_Keyval.rstr);
			//return append_bytes(_Keyval.str);
			
		}
	};

	template<>
	struct hash<types::date_t> {
		size_t operator() (const types::date_t& _Keyval) const noexcept {
			return ankerl::unordered_dense::hash<unsigned int>()(*(unsigned int*)(&_Keyval));
		}
	};

	template<>
	struct hash<types::time_t> {
		size_t operator() (const types::time_t& _Keyval) const noexcept {
			return ankerl::unordered_dense::hash<unsigned int>()(_Keyval.ms) ^ 
			ankerl::unordered_dense::hash<unsigned char>()(_Keyval.seconds) ^
			ankerl::unordered_dense::hash<unsigned char>()(_Keyval.minutes) ^
			ankerl::unordered_dense::hash<unsigned char>()(_Keyval.hours)
			;
		}
	};

	template<>
	struct hash<types::timestamp_t>{
		size_t operator() (const types::timestamp_t& _Keyval) const noexcept {
			return ankerl::unordered_dense::hash<types::date_t>()(_Keyval.date) ^ 
				ankerl::unordered_dense::hash<types::time_t>()(_Keyval.time);
		}
	};
#ifdef __SIZEOF_INT128__

	template<>
	struct hash<int128_struct>{
		size_t operator() (const int128_struct& _Keyval) const noexcept {
			return ankerl::unordered_dense::hash<uint64_t>()(_Keyval.__struct.low) ^ ankerl::unordered_dense::hash<uint64_t>()(_Keyval.__struct.high);
		}
	};
#endif
	template <class ...Types>
	struct hash<std::tuple<Types...>> : public hasher<Types...>{ };
}

struct aq_hashtable_value_t{
	uint32_t id;
	uint32_t cnt;
};