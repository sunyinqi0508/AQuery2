#pragma once

#include <functional>
#include <tuple>
template <class ...Types>
struct hasher {
	template <size_t i = 0> typename std::enable_if< i == sizeof...(Types), 
		size_t>::type hashi(const std::tuple<Types...>& record) const {
		return 0;
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
