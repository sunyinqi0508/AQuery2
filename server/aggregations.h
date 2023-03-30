#pragma once
#include "types.h"
#include "gc.h"
#include <utility>
#include <limits>
#include <deque>
#include <cmath>
#undef max
#undef min
template <class T, template<typename ...> class VT>
size_t count(const VT<T>& v) {
	return v.size;
}

template <class T>
constexpr static size_t count(const T&) { return 1; }

// TODO: Specializations for dt/str/none
template<class T, template<typename ...> class VT>
types::GetLongType<T>
sum(const VT<T>& v) {
	types::GetLongType<T> ret = 0;
	for (auto _v : v)
		ret += _v;
	return ret;
}
template<class T, template<typename ...> class VT>
//types::GetFPType<T> 
double avg(const VT<T>& v) {
	return (sum<T>(v) / static_cast<double>(v.size));
}

template<class T, template<typename ...> class VT, class Ret>
void sqrt(const VT<T>& v, Ret& ret) {
	for (uint32_t i = 0; i < v.size; ++i) 
		ret[i] = sqrt(v[i]);
}

template<class T, template<typename ...> class VT>
VT<double> sqrt(const VT<T>& v) {
	VT<double> ret(v.size);
	sqrt(v, ret);
	return ret;
}

template <class T>
T truncate(const T& v, const uint32_t precision) {
	auto multiplier = pow(10, precision);
	if (v >= std::numeric_limits<T>::max()/multiplier || 
			aq_fp_precision<T> <= precision)
		return v;
	else
		return round(v * multiplier)/multiplier;
}
template<class T, template<typename ...> class VT>
VT<T> truncate(const VT<T>& v, const uint32_t precision) {
	if (aq_fp_precision<T> <= precision)
		return v.subvec_memcpy();
	auto multiplier = pow(10, precision);
	auto max_truncate = std::numeric_limits<T>::max()/multiplier;
	VT<T> ret(v.size);
	for (uint32_t i = 0; i < v.size; ++i) { // round or trunc??
		ret[i] = v[i] < max_truncate ? round(v[i] * multiplier)/multiplier : v[i];
	}
	return ret;
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

// simplify this using a template std::binary_function<T, T, bool> = std::less;
template<class T, template<typename ...> class VT, class Ret>
void mins(const VT<T>& arr, Ret& ret) {
	const uint32_t& len = arr.size;
	T min = std::numeric_limits<T>::max();
	for (int i = 0; i < len; ++i) {
		if (arr[i] < min)
			min = arr[i];
		ret[i] = min;
	}
}

template<class T, template<typename ...> class VT>
decayed_t<VT, T> mins(const VT<T>& arr) {
	decayed_t<VT, T> ret(arr.size);
	mins(arr, ret);
	return ret;
}

template<class T, template<typename ...> class VT, class Ret>
void maxs(const VT<T>& arr, Ret& ret) {
	const uint32_t& len = arr.size;
	T max = std::numeric_limits<T>::min();
	for (int i = 0; i < len; ++i) {
		if (arr[i] > max)
			max = arr[i];
		ret[i] = max;
	}
}

template<class T, template<typename ...> class VT>
decayed_t<VT, T> maxs(const VT<T>& arr) {
	decayed_t<VT, T> ret(arr.size);
	maxs(arr, ret);
	return ret;
}

template<class T, template<typename ...> class VT, class Ret>
void minw(uint32_t w, const VT<T>& arr, Ret& ret) {
	const uint32_t& len = arr.size;
	std::deque<std::pair<T, uint32_t>> cache;
	for (int i = 0; i < len; ++i) {
		if (!cache.empty() && cache.front().second == i - w) cache.pop_front();
		
		while (!cache.empty() && cache.back().first > arr[i]) cache.pop_back();
		cache.push_back({ arr[i], i });
		ret[i] = cache.front().first;
	}
}

template<class T, template<typename ...> class VT>
decayed_t<VT, T> minw(uint32_t w, const VT<T>& arr) {
	decayed_t<VT, T> ret(arr.size);
	minw(w, arr, ret);
	return ret;
}

template<class T, template<typename ...> class VT, class Ret>
void maxw(uint32_t w, const VT<T>& arr, Ret& ret) {
	const uint32_t& len = arr.size;
	std::deque<std::pair<T, uint32_t>> cache;
	for (int i = 0; i < len; ++i) {
		if (!cache.empty() && cache.front().second == i - w) cache.pop_front();
		while (!cache.empty() && cache.back().first < arr[i]) cache.pop_back();
		cache.push_back({ arr[i], i });
		ret[i] = cache.front().first;
	}
}

template<class T, template<typename ...> class VT>
inline decayed_t<VT, T> maxw(uint32_t w, const VT<T>& arr) {
	decayed_t<VT, T> ret(arr.size);
	maxw(w, arr, ret);
	return ret;
}

template<class T, template<typename ...> class VT, class Ret>
void ratiow(uint32_t w, const VT<T>& arr, Ret& ret) {
	typedef std::decay_t<types::GetFPType<T>> FPType;
	uint32_t len = arr.size;
	if (arr.size <= w) 
		len = 1;
	w = w > len ? len : w;
	ret[0] = 0;
	for (uint32_t i = 0; i < w; ++i) 
		ret[i] = arr[i] / (FPType)arr[0];
	for (uint32_t i = w; i < arr.size; ++i) 
		ret[i] = arr[i] / (FPType) arr[i - w];
}

template<class T, template<typename ...> class VT>
inline decayed_t<VT, types::GetFPType<T>> ratiow(uint32_t w, const VT<T>& arr) {
	typedef std::decay_t<types::GetFPType<T>> FPType;
	decayed_t<VT, FPType> ret(arr.size);
	ratiow(w, arr, ret);
	return ret;
}

template<class T, template<typename ...> class VT>
inline decayed_t<VT, types::GetFPType<T>> ratios(const VT<T>& arr) { 
	return ratiow(1, arr);
}

template<class T, template<typename ...> class VT, class Ret>
inline void ratios(const VT<T>& arr, Ret& ret) { 
	return ratiow(1, arr, ret);
}

template<class T, template<typename ...> class VT, class Ret>
void sums(const VT<T>& arr, Ret& ret) {
	const uint32_t& len = arr.size;
	uint32_t i = 0;
	if (len) ret[i++] = arr[0];
	for (; i < len; ++i)
		ret[i] = ret[i - 1] + arr[i];
}

template<class T, template<typename ...> class VT>
inline decayed_t<VT, types::GetLongType<T>> sums(const VT<T>& arr) {
	decayed_t<VT, types::GetLongType<T>> ret(arr.size);
	sums(arr, ret);
	return ret;
}

template<class T, template<typename ...> class VT, class Ret>
void avgs(const VT<T>& arr, Ret& ret) {
	const uint32_t& len = arr.size;
	typedef types::GetFPType<types::GetLongType<T>> FPType;
	uint32_t i = 0;
	types::GetLongType<T> s;
	if (len) s = ret[i++] = arr[0];
	for (; i < len; ++i)
		ret[i] = (s += arr[i]) / (FPType)(i + 1);
}

template<class T, template<typename ...> class VT>
inline decayed_t<VT, types::GetFPType<types::GetLongType<T>>> avgs(const VT<T>& arr) {
	typedef types::GetFPType<types::GetLongType<T>> FPType;
	decayed_t<VT, FPType> ret(arr.size);
	avgs(arr, ret);
	return ret;
}

template<class T, template<typename ...> class VT, class Ret>
void sumw(uint32_t w, const VT<T>& arr, Ret& ret) {
	const uint32_t& len = arr.size;
	uint32_t i = 0;
	w = w > len ? len : w;
	if (len) ret[i++] = arr[0];
	for (; i < w; ++i)
		ret[i] = ret[i - 1] + arr[i];
	for (; i < len; ++i)
		ret[i] = ret[i - 1] + arr[i] - arr[i - w];
}

template<class T, template<typename ...> class VT>
decayed_t<VT, types::GetLongType<T>> sumw(uint32_t w, const VT<T>& arr) {
	decayed_t<VT, types::GetLongType<T>> ret(arr.size);
	sumw(w, arr, ret);
	return ret;
}

template<class T, template<typename ...> class VT, class Ret>
void avgw(uint32_t w, const VT<T>& arr, Ret& ret) {
	typedef types::GetFPType<types::GetLongType<T>> FPType;
	const uint32_t& len = arr.size;
	uint32_t i = 0;
	types::GetLongType<T> s{};
	w = w > len ? len : w;
	if (len) s = ret[i++] = arr[0];
	for (; i < w; ++i)
		ret[i] = (s += arr[i]) / (FPType)(i + 1);
	for (; i < len; ++i)
		ret[i] = ret[i - 1] + (arr[i] - arr[i - w]) / (FPType)w;
}

template<class T, template<typename ...> class VT>
inline decayed_t<VT, types::GetFPType<types::GetLongType<T>>> avgw(uint32_t w, const VT<T>& arr) {
	typedef types::GetFPType<types::GetLongType<T>> FPType;
	const uint32_t& len = arr.size;
	decayed_t<VT, FPType> ret(len);
	avgw(w, arr, ret);
	return ret;
}

template<class T, template<typename ...> class VT, class Ret, bool sd = false>
void varw(uint32_t w, const VT<T>& arr, 
	Ret& ret) {
	using FPType = types::GetFPType<types::GetLongType<T>>;
	const uint32_t& len = arr.size;
	uint32_t i = 0;
	types::GetLongType<T> s{};
	w = w > len ? len : w;
	FPType EnX {},  MnX{};
	if (len) {
		s = arr[0];
		MnX = 0;
		EnX = arr[0];
		ret[i++] = 0;
	}
	for (; i < len; ++i){
		s += arr[i];
		FPType _EnX = s / (FPType)(i + 1);
		MnX += (arr[i] - EnX) * (arr[i] - _EnX);
		EnX = _EnX;
		ret[i] = MnX / (FPType)(i + 1);
		if constexpr(sd) ret[i-1] = sqrt(ret[i-1]);
	}
	const float rw = 1.f / (float)w;
	s *= rw;	
	for (; i < len; ++i){
		const auto dw = arr[i] - arr[i - w - 1];
		const auto sw = arr[i] + arr[i - w - 1];
		const auto dex = dw * rw;
		ret[i] = ret[i-1] - dex*(s + s + dex - sw);
		if constexpr(sd) ret[i-1] = sqrt(ret[i-1]);
		s += dex;
	}
	if constexpr(sd) 
		if(i)
			ret[i-1] = sqrt(ret[i-1]);
}


template<class T, template<typename ...> class VT, bool sd = false>
inline decayed_t<VT, types::GetFPType<types::GetLongType<T>>> varw(uint32_t w, const VT<T>& arr) {
	using FPType = types::GetFPType<types::GetLongType<T>>;
	decayed_t<VT, FPType> ret(arr.size);
	varw<T, VT, decayed_t<VT, types::GetFPType<types::GetLongType<T>>>, sd>(w, arr, ret);
	return ret;
}

template<class T, template<typename ...> class VT>
types::GetFPType<types::GetLongType<decays<T>>> var(const VT<T>& arr) {
	typedef types::GetFPType<types::GetLongType<decays<T>>> FPType;
	const uint32_t& len = arr.size;
	uint32_t i = 0;
	types::GetLongType<T> s{0};
	types::GetLongType<T> ssq{0};
	if (len) {
		s = arr[0];
		ssq = arr[0] * arr[0];
	}
	for (; i < len; ++i){
		s += arr[i];
		ssq += arr[i] * arr[i];
	}
	return (ssq - s * s / (FPType)(len + 1)) / (FPType)(len + 1);
}

template<class T, template<typename ...> class VT, class Ret, bool sd = false>
void vars(const VT<T>& arr, Ret& ret) {
	typedef types::GetFPType<types::GetLongType<T>> FPType;
	const uint32_t& len = arr.size;
	uint32_t i = 0;
	types::GetLongType<T> s{};
	FPType MnX{};
	FPType EnX {};
	if (len) {
		s = arr[0];
		MnX = 0;
		EnX = arr[0];
		ret[i++] = 0;
	}
	for (; i < len; ++i){
		s += arr[i];
		FPType _EnX = s / (FPType)(i + 1);
		MnX += (arr[i] - EnX) * (arr[i] - _EnX);
		printf("%d %ld ", arr[i], MnX);
		EnX = _EnX;
		ret[i] = MnX / (FPType)(i + 1);
		if constexpr(sd) ret[i] = sqrt(ret[i]);
	}
}

template<class T, template<typename ...> class VT, bool sd = false>
inline decayed_t<VT, types::GetFPType<types::GetLongType<T>>> vars(const VT<T>& arr) {
	typedef types::GetFPType<types::GetLongType<T>> FPType;
	decayed_t<VT, FPType> ret(arr.size);
	vars<T, VT, decayed_t<VT, types::GetFPType<types::GetLongType<T>>>, sd>(arr, ret);
	return ret;
}

template<class T, template<typename ...> class VT, 
	class T2, template<typename ...> class VT2 
>
auto corr(const VT<T>& x, const VT2<T2>&y) {
	typedef types::Coercion<decays<T>, decays<T2>> InnerType;
	typedef types::GetLongType<InnerType> LongType;
	typedef types::GetFPType<LongType> FPType;
	// assert(x.size == y.size);
	const uint32_t& len = x.size;
	LongType sx{0}, sy{0}, sxy{0}, sx2{0}, sy2{0};
	for (uint32_t i = 0; i < len; ++i){
		sx += x[i];
		sx2 += x[i] * x[i];
		sy += y[i];
		sxy += x[i] * y[i];
		sy2 += y[i] * y[i];
	}
	return (sxy - FPType(sx*sy)) 
		/ 
	(len * sqrt(
			(sx2 - FPType(sx*sx)/len) * (sy2 - FPType(sy*sy)/len)
		)
	);
}


template<class T, template<typename ...> class VT>
inline types::GetFPType<types::GetLongType<decays<T>>> stddev(const VT<T>& arr) {
	return sqrt(var(arr));
}

template<class T, template<typename ...> class VT>
inline decayed_t<VT, types::GetFPType<types::GetLongType<T>>> stddevs(const VT<T>& arr) {
	return vars<T, VT, true>(arr);
}

template<class T, template<typename ...> class VT>
inline decayed_t<VT, types::GetFPType<types::GetLongType<T>>> stddevw(uint32_t w, const VT<T>& arr) {
	return varw<T, VT, true>(w, arr);
}

template<class T, template<typename ...> class VT, class Ret>
inline auto stddevs(const VT<T>& arr, Ret& ret) {
	return vars<T, VT, Ret, true>(arr, ret);
}

template<class T, template<typename ...> class VT, class Ret>
inline auto stddevw(uint32_t w, const VT<T>& arr, Ret& ret) {
	return varw<T, VT, Ret, true>(w, arr, ret);
}


// use getSignedType
template<class T, template<typename ...> class VT, class Ret>
void deltas(const VT<T>& arr, Ret& ret) {
	const uint32_t& len = arr.size;
	uint32_t i = 0;
	if (len) ret[i++] = 0;
	for (; i < len; ++i)
		ret[i] = arr[i] - arr[i - 1];
}

template<class T, template<typename ...> class VT>
inline decayed_t<VT, T> deltas(const VT<T>& arr) {
	decayed_t<VT, T> ret(arr.size);
	deltas(arr, ret);
	return ret;
}

template<class T, template<typename ...> class VT, class Ret>
void prev(const VT<T>& arr, Ret& ret) {
	const uint32_t& len = arr.size;
	uint32_t i = 0;
	if (len) ret[i++] = arr[0];
	for (; i < len; ++i)
		ret[i] = arr[i - 1];
}

template<class T, template<typename ...> class VT>
inline decayed_t<VT, T> prev(const VT<T>& arr) {
	decayed_t<VT, T> ret(arr.size);
	prev(arr, ret);
	return ret;
}

template<class T, template<typename ...> class VT, class Ret>
void aggnext(const VT<T>& arr, Ret& ret) {
	const uint32_t& len = arr.size;
	uint32_t i = 1;
	for (; i < len; ++i)
		ret[i - 1] = arr[i];
	if (len > 0) ret[len - 1] = arr[len - 1];
}

template<class T, template<typename ...> class VT>
inline decayed_t<VT, T> aggnext(const VT<T>& arr) {
	decayed_t<VT, T> ret(arr.size);
	aggnext(arr, ret);
	return ret;
}

template<class T, template<typename ...> class VT>
T last(const VT<T>& arr) {
	if (!arr.size) return 0;
	return arr[arr.size - 1];
}

template<class T, template<typename ...> class VT>
T first(const VT<T>& arr) {
	if (!arr.size) return 0;
	return arr[0];
}

#define __DEFAULT_AGGREGATE_FUNCTION__(NAME, RET) \
template <class T> constexpr T NAME(const T& v) { return RET; }

// non-aggreation count. E.g. SELECT COUNT(col) from table; 
template <class T> constexpr T count(const T&) { return 1; }
template <class T> constexpr T var(const T&) { return 0; }
template <class T> constexpr T vars(const T&) { return 0; }
template <class T> constexpr T varw(uint32_t, const T&) { return 0; }
template <class T> constexpr T stddev(const T&) { return 0; }
template <class T> constexpr T stddevs(const T&) { return 0; }
template <class T> constexpr T stddevw(uint32_t, const T&) { return 0; }
template <class T> constexpr T max(const T& v) { return v; }
template <class T> constexpr T min(const T& v) { return v; }
template <class T> constexpr T avg(const T& v) { return v; }
template <class T> constexpr T sum(const T& v) { return v; }
template <class T> constexpr T maxw(uint32_t, const T& v) { return v; }
template <class T> constexpr T minw(uint32_t, const T& v) { return v; }
template <class T> constexpr T avgw(uint32_t, const T& v) { return v; }
template <class T> constexpr T sumw(uint32_t, const T& v) { return v; }
template <class T> constexpr T ratiow(uint32_t, const T&) { return 1; }
template <class T> constexpr T maxs(const T& v) { return v; }
template <class T> constexpr T mins(const T& v) { return v; }
template <class T> constexpr T avgs(const T& v) { return v; }
template <class T> constexpr T sums(const T& v) { return v; }
template <class T> constexpr T last(const T& v) { return v; }
template <class T> constexpr T prev(const T& v) { return v; }
template <class T> constexpr T aggnext(const T& v) { return v; }
template <class T> constexpr T daltas(const T&) { return 0; }
template <class T> constexpr T ratios(const T&) { return 1; }
