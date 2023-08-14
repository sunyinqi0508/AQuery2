/*
* (C) Bill Sun 2022 - 2023
*/

#pragma once

#include <type_traits>
#include <tuple>
#include <functional>
#include <string_view>
#include "types.h"
#include "vector_type.hpp"
// #include "robin_hood.h"
#include "unordered_dense.h"

template<typename Key, typename Val>
using aq_map = ankerl::unordered_dense::map<Key, Val>;

template<typename Key>
using aq_set = ankerl::unordered_dense::set<Key>;

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

	template<typename... Keys_t>
	inline void hashtable_push_all(Keys_t& ... keys, uint32_t len) {
		for(uint32_t i = 0; i < len; ++i) 
			reversemap[i] = ankerl::unordered_dense::set<Key, Hash>::hashtable_push(keys[i]...);
		for(uint32_t i = 0; i < len; ++i) 
			++ht_base[reversemap[i]]; 
	}
	inline void hashtable_push(Key&& k, uint32_t i){
		reversemap[i] = ankerl::unordered_dense::set<Key, Hash>::hashtable_push(k);
		++ht_base[reversemap[i]]; // do this seperately?
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


template <
	typename ValueType = uint32_t,
	int PerfectHashingThreshold = 12
>
struct PerfectHashTable {
	using key_t = std::conditional_t<PerfectHashingThreshold <= 8, uint8_t,
		std::conditional_t<PerfectHashingThreshold <= 16, uint16_t,
		std::conditional_t<PerfectHashingThreshold <= 32, uint32_t,
		uint64_t
		>>>;
	constexpr static uint32_t tbl_sz = 1 << PerfectHashingThreshold;
	template <typename ... Types, template <typename> class VT>
	static vector_type<uint32_t>*
	construct(VT<Types>&... args) { // construct a hash set
		// AQTmr();
		int n_cols, n_rows = 0;

		((n_cols = args.size), ...);
		static_assert(
			(sizeof...(Types) < PerfectHashingThreshold) &&
			(std::is_integral_v<Types> && ...),
			"Types must be integral and less than 12 wide in total."
			);
		key_t* 
		hash_values = static_cast<key_t*>(
				calloc(n_cols, sizeof(key_t))
		);
		auto get_hash = [&hash_values](auto& arg, int idx) {
			
			if (idx > 0) {
#pragma omp simd
				for (uint32_t i = 0; i < arg.size; ++i) {
					hash_values[i] =
						(hash_values[i] << arg.stats.bits) +
						(arg.container[i] - arg.stats.minima);
				}
			}
			else {
#pragma omp simd
				for (uint32_t i = 0; i < arg.size; ++i) {
						hash_values[i] = arg.container[i] - arg.stats.minima;
					}
				}
			};
		int idx = 0;
		(get_hash(args, idx++), ...);
		uint32_t cnt[tbl_sz];
		uint32_t n_grps = 0;
		memset(cnt, 0, tbl_sz * sizeof(tbl_sz));
#pragma omp simd
		for (uint32_t i = 0; i < n_cols; ++i) {
			++cnt[hash_values[i]];
		}
		ValueType grp_ids[tbl_sz];
#pragma omp simd
		for (ValueType i = 0; i < tbl_sz; ++i) {
			if (cnt[i] != 0) {
				cnt[n_grps] = cnt[i];
				grp_ids[i] = n_grps++;
			}
		}
		uint32_t* idxs = static_cast<uint32_t*>(
			malloc(n_cols * sizeof(uint32_t))
		);
		uint32_t** idxs_ptr = static_cast<uint32_t**>(
			malloc(n_grps * sizeof(uint32_t*))
		);
		idxs_ptr[0] = idxs;
#ifdef _MSCVER
#pragma omp simd
#endif
		for (int i = 1; i < n_grps; ++i) {
			idxs_ptr[i] = idxs_ptr[i - 1] + cnt[i - 1];
		}
#pragma omp simd 
		for (int i = 0; i < n_cols; ++i) {
			*(idxs_ptr[grp_ids[hash_values[i]]]++) = i;
		}
		vector_type<uint32_t>* idxs_vec = static_cast<vector_type<uint32_t>*>(
			malloc(n_grps * sizeof(vector_type<uint32_t>))
		);
#pragma omp simd
		for (int i = 0; i < n_grps; ++i) {
			idxs_vec[i].container = idxs_ptr[i];
			idxs_vec[i].size = cnt[i];
		}
		free(hash_values);
		return idxs_vec;
	}
};
