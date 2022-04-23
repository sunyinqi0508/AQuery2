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
int _main();
int main(int argc, char** argv) {
    printf("%d %s\n", argc, argv[1]);
    const char* shmname;
    if (argc <= 1)
        return _main();
    else
        shmname = argv[1];
    SharedMemory shm = SharedMemory(shmname);
    if (!shm.pData)
        return 1;
    bool &running = static_cast<bool*>(shm.pData)[0], 
        &ready = static_cast<bool*>(shm.pData)[1];
    Context *cxt = new Context();
    using namespace std::chrono_literals;
    printf("running: %s\n", running? "true":"false");
    printf("ready: %s\n", ready? "true":"false");
    while (running) {
        std::this_thread::sleep_for(1ms);
        if(ready){
            printf("running: %s\n", running? "true":"false");
            printf("ready: %s\n", ready? "true":"false");
            void* handle = dlopen("./dll.so", RTLD_LAZY);
            printf("handle: %x\n", handle);
            if (handle) {
                printf("inner\n");
                code_snippet c = reinterpret_cast<code_snippet>(dlsym(handle, "dllmain"));
                printf("routine: %x\n", c);
                if (c) {
                    printf("inner\n");
                    printf("return: %d\n", c(cxt));
                }
                dlclose(handle);
            }
            ready = false;
        }
    }
    shm.FreeMemoryMap();
    return 0;
}
#include "utils.h"
int _main()
{

    //vector_type<int> t;
    //t = 1;
    //t.emplace_back(2);
    //print(t);
    //return 0;
    puts(cpp_17 ?"true":"false");
    void* handle = dlopen("dll.so", RTLD_LAZY);
    printf("handle: %x\n", handle);
    Context* cxt = new Context();
    if (handle) {
        printf("inner\n");
        code_snippet c = reinterpret_cast<code_snippet>(dlsym(handle, "dllmain"));
        printf("routine: %x\n", c);
        if (c) {
            printf("inner\n");
            printf("return: %d\n", c(cxt));
        }
        dlclose(handle);
    }
    static_assert(std::is_same_v<decltype(fill_integer_array<5, 1>()), std::integer_sequence<bool, 1,1,1,1,1>>, "");
    return 0;
    
}

