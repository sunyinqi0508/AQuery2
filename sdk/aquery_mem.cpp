#include "aquery.h"

#include <memory>
#include <cstdlib>
#include <unordered_map>

Session* session;


void* Aalloc(size_t sz, deallocator_t deallocator){
    void* mem =  malloc(sz);
    auto memmap = (std::unordered_map<void*, deallocator_t>*) session->memory_map;
    memmap->operator[](mem) = deallocator;
    return mem;
}

void Afree(void* mem){
    auto memmap = (std::unordered_map<void*, deallocator_t>*) session->memory_map;
    memmap->operator[](mem)(mem);
    memmap->erase(mem);
}

void register_memory(void* ptr, deallocator_t deallocator){
    auto memmap = (std::unordered_map<void*, deallocator_t>*) session->memory_map;
    memmap->operator[](ptr) = deallocator;
}

__AQEXPORT__(void) init_session(Context* cxt){
    session = &cxt->current;
}
