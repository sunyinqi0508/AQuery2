#pragma once
#include "./server/libaquery.h"
#include "./server/aggregations.h"

auto covariances = [](auto x, auto y, auto w) {
	static auto xmeans=0.0;static auto ymeans=0.0; static auto cnt=0;
	auto reset = [=]() { xmeans=0.0, ymeans=0.0, cnt=0; };	
	auto call = [](decltype(x) x, decltype(y) y, decltype(w) w){  
		if((cnt < w)) {			
			xmeans += x;
			ymeans += y;
			cnt += 1;
		}
		y = (x - xmeans);
		return avg(((x.subvec((x - w), x) - xmeans) * (y.subvec((y - w), y) - ymeans)));	
	};
	return std::make_pair(reset, call);
};

