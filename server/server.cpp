#include "../csv.h"
#include <iostream>
#include <string>
//#include <thread>
#include <chrono>

#include "libaquery.h"
#ifdef _WIN32
#include "winhelper.h"
#else 
#include <dlfcn.h>
#include <fcntl.h>
#include <sys/mman.h>
struct SharedMemory
{
    int hFileMap;
    void* pData;
    SharedMemory(const char* fname) {
        hFileMap = open(fname, O_RDWR, 0);
        if (hFileMap != -1)
            pData = mmap(NULL, 8, PROT_READ | PROT_WRITE, MAP_SHARED, hFileMap, 0);
        else 
            pData = 0;
    }
    void FreeMemoryMap() {

    }
};
#endif
struct thread_context{

}v;
void daemon(thread_context* c) {

}
#include "aggregations.h"
typedef int (*code_snippet)(void*);
int test_main();

int dll_main(int argc, char** argv, Context* cxt){
    Config *cfg = reinterpret_cast<Config *>(argv[0]);

    auto buf_szs = cfg->buffer_sizes;
    void** buffers = (void**)malloc(sizeof(void*) * cfg->n_buffers);
    for (int i = 0; i < cfg->n_buffers; i++) 
        buffers[i] = static_cast<void *>(argv[i + 1]);

    cxt->buffers = buffers;
    cxt->cfg = cfg;
    cxt->n_buffers = cfg->n_buffers;
    cxt->sz_bufs = buf_szs;

    while(cfg->running){
        if (cfg->new_query) {
            void* handle = dlopen("./dll.so", RTLD_LAZY);
            code_snippet c = reinterpret_cast<code_snippet>(dlsym(handle, "dllmain"));
            c(cxt);
            dlclose(handle);
            cfg->new_query = 0;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    return 0;
}

extern "C" int __DLLEXPORT__ main(int argc, char** argv) {
    Context* cxt = new Context();
    cxt->log("%d %s\n", argc, argv[1]);
    
    const char* shmname;
    if (argc < 0)
        return dll_main(argc, argv, cxt);
    else if (argc <= 1)
        return test_main();
    else
        shmname = argv[1];
    SharedMemory shm = SharedMemory(shmname);
    if (!shm.pData)
        return 1;
    bool &running = static_cast<bool*>(shm.pData)[0], 
        &ready = static_cast<bool*>(shm.pData)[1];
    using namespace std::chrono_literals;
    cxt->log("running: %s\n", running? "true":"false");
    cxt->log("ready: %s\n", ready? "true":"false");
    while (running) {
        std::this_thread::sleep_for(1ms);
        if(ready){
            cxt->log("running: %s\n", running? "true":"false");
            cxt->log("ready: %s\n", ready? "true":"false");
            void* handle = dlopen("./dll.so", RTLD_LAZY);
            cxt->log("handle: %lx\n", handle);
            if (handle) {
                cxt->log("inner\n");
                code_snippet c = reinterpret_cast<code_snippet>(dlsym(handle, "dllmain"));
                cxt->log("routine: %lx\n", c);
                if (c) {
                    cxt->log("inner\n");
                    cxt->err("return: %d\n", c(cxt));
                }
            }
            ready = false;
        }
    }
    shm.FreeMemoryMap();
    return 0;
}
#include "utils.h"
int test_main()
{
    //vector_type<int> t;
    //t = 1;
    //t.emplace_back(2);
    //print(t);
    //return 0;
    Context* cxt = new Context();
    cxt->log_level = LOG_INFO;
    puts(cpp_17 ?"true":"false");
    void* handle = dlopen("dll.so", RTLD_LAZY);
    cxt->log("handle: %llx\n", handle);
    if (handle) {
        cxt->log("inner\n");
        code_snippet c = reinterpret_cast<code_snippet>(dlsym(handle, "dllmain"));
        cxt->log("routine: %llx\n", c);
        if (c) {
            cxt->log("inner\n");
            cxt->log("return: %d\n", c(cxt));
        }
        dlclose(handle);
    }
    //static_assert(std::is_same_v<decltype(fill_integer_array<5, 1>()), std::integer_sequence<bool, 1,1,1,1,1>>, "");
    return 0;
    std::unordered_map<int, int> a;
}

