#include "../server/libaquery.h"


extern void* Aalloc(size_t sz);
extern int Afree(void * mem);
extern size_t register_memory(void* ptr, void(dealloc)(void*));

struct Session{
    struct Statistic{
        size_t total_active;
        size_t cnt_object;
        size_t total_alloc;
    };
    void* memory_map;
};
#define EXPORT __DLLEXPORT__