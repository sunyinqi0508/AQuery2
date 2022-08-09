#pragma once
#include "./server/libaquery.h"
#include "./server/aggregations.h"

auto covariances2 = [](auto x, auto y, auto w, uint32_t _builtin_len, auto& _builtin_ret) {
	auto xmeans = 0.0;
	auto ymeans = 0.0;
	auto l = _builtin_len;
	if((l > 0)) {
		xmeans = x[0];
		ymeans = y[0];
		_builtin_ret[0] = 0.0;
	}
	if((w > l))
		w = l;
	for(auto  i = 1,  j = 0; (i < w); i = (i + 1))  {
		xmeans += x[i];
		ymeans += y[i];
		_builtin_ret[i] = avg(((x.subvec(0, i) - (xmeans / i)) * (y.subvec(0, i) - (ymeans / i))));
	}
	xmeans /= w;
	ymeans /= w;
	for(auto i = w; (i < l); i += 1)  {
		xmeans += ((x[i] - x[(i - w)]) / w);
		ymeans += ((y[i] - y[(i - w)]) / w);
		_builtin_ret[i] = avg(((x.subvec((i - w), i) - xmeans) * (y.subvec((i - w), i) - ymeans)));
	}
	return ;
};

auto covariances2_gettype = [](auto x, auto y, auto w) {
	uint32_t _builtin_len = 0;
	auto xmeans = 0.0;
	auto ymeans = 0.0;
	auto l = _builtin_len;
	if((l > 0)) {
		xmeans = x[0];
		ymeans = y[0];
		return 0.0;
	}
	if((w > l))
		w = l;
	for(auto  i = 1,  j = 0; (i < w); i = (i + 1))  {
		xmeans += x[i];
		ymeans += y[i];
		return avg(((x.subvec(0, i) - (xmeans / i)) * (y.subvec(0, i) - (ymeans / i))));
	}
	xmeans /= w;
	ymeans /= w;
	for(auto i = w; (i < l); i += 1)  {
		xmeans += ((x[i] - x[(i - w)]) / w);
		ymeans += ((y[i] - y[(i - w)]) / w);
		return avg(((x.subvec((i - w), i) - xmeans) * (y.subvec((i - w), i) - ymeans)));
	}
};

