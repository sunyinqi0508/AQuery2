#include "aquery.h"

#include <memory>
#include <stdlib>
#include <unordered_set>

Session* session;
void* Aalloc(size_t sz){
    void mem =  malloc(sz);
    auto memmap = (std::unordered_set<void*>*) session->memory_map;
    memmap->insert(mem);
    return mem;
}

int Afree(void* mem){
    auto memmap = (std::unordered_set<void*>*) session->memory_map;
    memmap->erase(mem);
    return free(mem);
}
