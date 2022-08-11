#include "aquery.h"

#include <memory>
#include <stdlib>
#include <unordered_map>

Session* session;
void init_session(){

}

void end_session(){

}

void* Aalloc(size_t sz){
    void* mem =  malloc(sz);
    auto memmap = (std::unordered_map<void*>*) session->memory_map;
    memmap->insert(mem);
    return mem;
}

int Afree(void* mem){
    auto memmap = (std::unordered_map<void*>*) session->memory_map;
    memmap->erase(mem);
    return free(mem);
}

void register_memory(void* ptr, void(dealloc)(void*)){
    auto memmap = (std::unordered_map<void*>*) session->memory_map;
    memmap->insert(ptr);
}

