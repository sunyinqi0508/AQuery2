#pragma once
#include "./server/libaquery.h"
#include "./server/aggregations.h"

auto covariance = [](auto x, auto y) {
	auto xmean = avg(x);
	auto ymean = avg(y);
	return avg(((x - xmean) * (y - ymean)));
};

auto sd = [](auto x) {
	return sqrt(covariance(x, x));
};

auto paircorr = [](auto x, auto y) {
	return (covariance(x, y) / (sd(x) * sd(y)));
};

