#pragma once
#include <string>
#include <ctime>

template<int cnt, int begin = 0, int interval = 1>
struct const_range {
	int arr[cnt];
	constexpr const_range() {
		for (int i = begin, n = 0; n < cnt; ++n, i += interval)
			arr[n] = i;
	}
	const int* begin() const {
		return arr;
	}
	const int* end() const {
		return arr + cnt;
	}
};