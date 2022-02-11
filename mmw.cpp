#include <cstdint>
#include <deque>

using std::uint32_t;

template<class T, bool minmax>
void running(void *array, uint32_t len, uint32_t w){
	T* arr = static_cast<T*> (array);
	std::deque<std::pair<T, uint32_t>> cache;
	for(int i = 0; i < len; ++i){
		if(!cache.empty() && cache.front().second == i-w) cache.pop_front();
		if constexpr(minmax)
			while(!cache.empty() && cache.back().first>arr[i]) cache.pop_back();
		else
			while(!cache.empty() && cache.back().first<arr[i]) cache.pop_back();
		cache.push_back({arr[i], i});
		arr[i] = cache.front().first;
	}
}
template<class T>
inline void mm(void *array, uint32_t len, uint32_t w, bool mm){
	mm?	running<T, true>(array, len, w) : running<T, false>(array, len, w);
}
extern "C" { 
	#include <stdio.h> 

	int mmw(void *array, unsigned long long misc[]){
		char _ty = misc[0];
		uint32_t len = misc[1];
		uint32_t w = misc[2];
		bool minmax = misc[3]-0x10000;
		switch(_ty){
			case 'F': mm<double>(array, len, w, minmax); break;
			case 'C': case 'G': mm<unsigned char>(array, len, w, minmax); break;
			case 'H': mm<unsigned short>(array, len, w, minmax); break;
			case 'D': case 'I': mm<unsigned int>(array, len, w, minmax); break;
			case 'T': case 'J': mm<long long>(array, len, w, minmax); break;
			case 'L': if(len == 0) break;
			default: printf("nyi %c\n", _ty);
		}
		return 0; 
	}
}
