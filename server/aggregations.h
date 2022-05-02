#pragma once
#include "types.h"
#include <utility>
#include <limits>
#include <deque>
template <class T, template<typename ...> class VT>
size_t count(const VT<T>& v) {
	return v.size;
}

template <class T>
constexpr static inline size_t count(const T&) { return 1; }

// TODO: Specializations for dt/str/none
template<class T, template<typename ...> class VT>
types::GetLongType<T> sum(const VT<T>& v) {
	types::GetLongType<T> ret = 0;
	for (const auto& _v : v)
		ret += _v;
	return ret;
}
template<class T, template<typename ...> class VT>
types::GetFPType<T> avg(const VT<T>& v) {
	return static_cast<types::GetFPType<T>>(
		sum<T>(v) / static_cast<long double>(v.size));
}
template <class T, template<typename ...> class VT>
T max(const VT<T>& v) {
	T max_v = std::numeric_limits<T>::min();
	for (const auto& _v : v)
		max_v = max_v > _v ? max_v : _v;
	return max_v;
}
template <class T, template<typename ...> class VT>
T min(const VT<T>& v) {
	T min_v = std::numeric_limits<T>::max();
	for (const auto& _v : v)
		min_v = min_v < _v ? min_v : _v;
	return min_v;
}
template<class T, template<typename ...> class VT>
decayed_t<VT,T> mins(const VT<T>& arr) {
	const uint32_t& len = arr.size;
	std::deque<std::pair<T, uint32_t>> cache;
	decayed_t<VT,T> ret(len);
	T min = std::numeric_limits<T>::max();
	for (int i = 0; i < len; ++i) {
		if (arr[i] < min)
			min = arr[i];
		ret[i] = min;
	}
	return ret;
}
template<class T, template<typename ...> class VT>
decayed_t<VT,T> maxs(const VT<T>& arr) {
	const uint32_t& len = arr.size;
	decayed_t<VT,T> ret(len);
	T max = std::numeric_limits<T>::min();
	for (int i = 0; i < len; ++i) {
		if (arr[i] > max)
			max = arr[i];
		ret[i] = max;
	}
	return ret;
}

template<class T, template<typename ...> class VT>
decayed_t<VT,T> minw(uint32_t w, const VT<T>& arr) {
	const uint32_t& len = arr.size;
	decayed_t<VT,T> ret{len};
	std::deque<std::pair<T, uint32_t>> cache;
	for (int i = 0; i < len; ++i) {
		if (!cache.empty() && cache.front().second == i - w) cache.pop_front();
		while (!cache.empty() && cache.back().first > arr[i]) cache.pop_back();
		cache.push_back({ arr[i], i });
		ret[i] = cache.front().first;
	}
	return ret;
}

template<class T, template<typename ...> class VT>
decayed_t<VT,T> maxw(uint32_t w, const VT<T>& arr) {
	const uint32_t& len = arr.size;
	decayed_t<VT, T> ret(len);
	std::deque<std::pair<T, uint32_t>> cache;
	for (int i = 0; i < len; ++i) {
		if (!cache.empty() && cache.front().second == i - w) cache.pop_front();
		while (!cache.empty() && cache.back().first > arr[i]) cache.pop_back();
		cache.push_back({ arr[i], i });
		arr[i] = cache.front().first;
	}
	return ret;
}

template<class T, template<typename ...> class VT>
decayed_t<VT, types::GetLongType<T>> sums(const VT<T>& arr) {
	const uint32_t& len = arr.size;
	decayed_t<VT, types::GetLongType<T>> ret(len);
	uint32_t i = 0;
	if(len) ret[i++] = arr[0];
	for (; i < len; ++i) 
		ret[i] = ret[i-1] + arr[i];
	return ret;
}
template<class T, template<typename ...> class VT>
decayed_t<VT, types::GetFPType<T>> avgs(const VT<T>& arr) {
	const uint32_t& len = arr.size;
	typedef types::GetFPType<T> FPType;
	decayed_t<VT, FPType> ret(len);
	uint32_t i = 0;
	types::GetLongType<T> s;
	if(len) s = ret[i++] = arr[0];
	for (; i < len; ++i) 
		ret[i] = (s+=arr[i])/(FPType)(i+1);
	return ret;
}
template<class T, template<typename ...> class VT>
decayed_t<VT, types::GetLongType<T>> sumw(uint32_t w, const VT<T>& arr) {
	const uint32_t& len = arr.size;
	decayed_t<VT, types::GetLongType<T>> ret(len);
	uint32_t i = 0;
	w = w > len ? len : w;
	if(len) ret[i++] = arr[0];
	for (; i < w; ++i) 
		ret[i] = ret[i-1] + arr[i];
	for (; i < len; ++i) 
		ret[i] = ret[i-1] + arr[i] - arr[i-w];
	return ret;
}
template<class T, template<typename ...> class VT>
decayed_t<VT, types::GetFPType<T>> avgw(uint32_t w, const VT<T>& arr) {
	typedef types::GetFPType<T> FPType;
	const uint32_t& len = arr.size;
	decayed_t<VT, FPType> ret(len);
	uint32_t i = 0;
	types::GetLongType<T> s{};
	w = w > len ? len : w;
	if(len)	s = ret[i++] = arr[0];
	for (; i < w; ++i) 
		ret[i] = (s += arr[i])/(FPType)(i+1);
	for (; i < len; ++i) 
		ret[i] = ret[i-1] + (arr[i] - arr[i-w])/(FPType)w;
	return ret;
}

template <class T> constexpr inline T count(const T& v) { return 1; }
template <class T> constexpr inline T max(const T& v) { return v; }
template <class T> constexpr inline T min(const T& v) { return v; }
template <class T> constexpr inline T avg(const T& v) { return v; }
template <class T> constexpr inline T sum(const T& v) { return v; }
template <class T> constexpr inline T maxw(uint32_t, const T& v) { return v; }
template <class T> constexpr inline T minw(uint32_t, const T& v) { return v; }
template <class T> constexpr inline T avgw(uint32_t, const T& v) { return v; }
template <class T> constexpr inline T sumw(uint32_t, const T& v) { return v; }
template <class T> constexpr inline T maxs(const T& v) { return v; }
template <class T> constexpr inline T mins(const T& v) { return v; }
template <class T> constexpr inline T avgs(const T& v) { return v; }
template <class T> constexpr inline T sums(const T& v) { return v; }
