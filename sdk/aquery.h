#include "../server/libaquery.h"


extern void* Aalloc(size_t sz);
extern int Afree(void * mem);

template <typename T>
size_t register_memory(T* ptr, void(dealloc)(void*)){
    [](void* m){ auto _m = static_cast<T*>(m); delete _m; };
}
struct Session{
    struct Statistic{
        size_t total_active;
        size_t cnt_object;
        size_t total_alloc;
    };
    void* memory_map;
};
#define EXPORT __DLLEXPORT__