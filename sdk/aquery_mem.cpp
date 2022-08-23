#include "aquery.h"

#include <memory>
#include <stdlib>
#include <unordered_map>

Session* session;


void* Aalloc(size_t sz, deallocator_t deallocator){
    void* mem =  malloc(sz);
    auto memmap = (std::unordered_map<void*, dealloctor_t>*) session->memory_map;
    memmap[mem] = deallocator;
    return mem;
}

void Afree(void* mem){
    auto memmap = (std::unordered_map<void*, dealloctor_t>*) session->memory_map;
    memmap[mem](mem);
    memmap->erase(mem);
}

void register_memory(void* ptr, deallocator_t deallocator){
    auto memmap = (std::unordered_map<void*, dealloctor_t>*) session->memory_map;
    memmap[ptr] = deallocator;
}

