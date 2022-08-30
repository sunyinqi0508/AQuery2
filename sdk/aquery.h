#include "../server/libaquery.h"

typedef void (*dealloctor_t) (void*);

extern void* Aalloc(size_t sz);
extern void Afree(void * mem);
extern size_t register_memory(void* ptr, dealloctor_t deallocator);

struct Session{
    struct Statistic{
        size_t total_active;
        size_t cnt_object;
        size_t total_alloc;
    };
    void* memory_map;
};
