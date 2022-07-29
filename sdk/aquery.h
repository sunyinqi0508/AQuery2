#include "../server/libaquery.h"


extern void* Aalloc(size_t sz);
extern int Afree(void * mem);

template <typename T>
size_t register_memory(T* ptr){
    [](void* m){ auto _m = static_cast<T*>(m); delete _m; };
}

#define EXPORT __DLLEXPORT__