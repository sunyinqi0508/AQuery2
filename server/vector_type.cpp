#include "vector_type.hpp"
#include <iostream>
template<typename _Ty>
inline void vector_type<_Ty>::out(uint32_t n, const char* sep) const
{
	n = n > size ? size : n;
	std::cout << '(';
	{	
		uint32_t i = 0;
		for (; i < n - 1; ++i)
			std::cout << this->operator[](i) << sep;
		std::cout << this->operator[](i);
	}
	std::cout << ')';
}