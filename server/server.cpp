#include "../csv.h"
#include <iostream>
#include <string>
//#include <thread>
#include <chrono>

#include "libaquery.h"
#include "monetdb_conn.h"
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

int n_recv = 0;
char** n_recvd = nullptr;

extern "C" void __DLLEXPORT__ receive_args(int argc, char**argv){
    n_recv = argc;
    n_recvd = argv;
}
enum BinaryInfo_t {
	MSVC, MSYS, GCC, CLANG, AppleClang
};
extern "C" int __DLLEXPORT__ binary_info() {
#if defined(_MSC_VER) && !defined (__llvm__)
	return MSVC;
#elif defined(__CYGWIN__) || defined(__MINGW32__) || defined(__MINGW64__)
	return MSYS;
#elif defined(__GNUC__)
    return GCC;
#else
	return AppleClang;
#endif
}
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
    cxt->alt_server = NULL;

    while(cfg->running){
        if (cfg->new_query) {

            if (cfg->backend_type == BACKEND_MonetDB){
                if (cxt->alt_server == 0)
                    cxt->alt_server = new Server(cxt);
                Server* server = reinterpret_cast<Server*>(cxt->alt_server);
                if(n_recv > 0){
                    for(int i = 0; i < n_recv; ++i)
                    {
                        server->exec(n_recvd[i]);
                        printf("Exec Q%d: %s\n", i, n_recvd[i]);
                    }
                    n_recv = 0;
                }
                if(server->last_error == nullptr){
                    
                }   
                else{
                    server->last_error = nullptr;
                    continue;
                } 
            }
            
            // puts(cfg->has_dll ? "true" : "false");
            if (cfg->backend_type == BACKEND_AQuery || cfg->has_dll) {
                void* handle = dlopen("./dll.so", RTLD_LAZY);
                code_snippet c = reinterpret_cast<code_snippet>(dlsym(handle, "dllmain"));
                c(cxt);
                dlclose(handle);
            }
            cfg->new_query = 0;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    return 0;
}
//
//extern "C" int __DLLEXPORT__ main(int argc, char** argv) {
//
//    puts("running");
//    Context* cxt = new Context();
//    cxt->log("%d %s\n", argc, argv[1]);
//    
//    const char* shmname;
//    if (argc < 0)
//        return dll_main(argc, argv, cxt);
//    else if (argc <= 1)
//        return test_main();
//    else
//        shmname = argv[1];
//    SharedMemory shm = SharedMemory(shmname);
//    if (!shm.pData)
//        return 1;
//    bool &running = static_cast<bool*>(shm.pData)[0], 
//        &ready = static_cast<bool*>(shm.pData)[1];
//    using namespace std::chrono_literals;
//    cxt->log("running: %s\n", running? "true":"false");
//    cxt->log("ready: %s\n", ready? "true":"false");
//    while (running) {
//        std::this_thread::sleep_for(1ms);
//        if(ready){
//            cxt->log("running: %s\n", running? "true":"false");
//            cxt->log("ready: %s\n", ready? "true":"false");
//            void* handle = dlopen("./dll.so", RTLD_LAZY);
//            cxt->log("handle: %lx\n", handle);
//            if (handle) {
//                cxt->log("inner\n");
//                code_snippet c = reinterpret_cast<code_snippet>(dlsym(handle, "dllmain"));
//                cxt->log("routine: %lx\n", c);
//                if (c) {
//                    cxt->log("inner\n");
//                    cxt->err("return: %d\n", c(cxt));
//                }
//            }
//            ready = false;
//        }
//    }
//    shm.FreeMemoryMap();
//    return 0;
//}
#include "utils.h"
int test_main()
{
    //vector_type<int> t;
    //t = 1;
    //t.emplace_back(2);
    //print(t);
    //return 0;
    Context* cxt = new Context();
    if (cxt->alt_server == 0)
        cxt->alt_server = new Server(cxt);
    Server* server = reinterpret_cast<Server*>(cxt->alt_server);
    const char* qs[]= {
        "CREATE TABLE tt(a INT, b INT, c INT, d INT);",
        "COPY OFFSET 2 INTO tt FROM  'D:/gg/AQuery++/test.csv'  ON SERVER    USING DELIMITERS  ',';",
        "CREATE TABLE sale(Mont INT, sales INT);",
		"COPY OFFSET 2 INTO sale FROM  'D:/gg/AQuery++/moving_avg.csv'  ON SERVER    USING DELIMITERS  ',';",
		"SELECT a FROM tt, sale WHERE a = Mont  ;"
    };
    n_recv = sizeof(qs)/(sizeof (char*));
	n_recvd = const_cast<char**>(qs);
    if (n_recv > 0) {
        for (int i = 0; i < n_recv; ++i)
        {
            server->exec(n_recvd[i]);
            printf("Exec Q%d: %s\n", i, n_recvd[i]);
        }
        n_recv = 0;
    }

    cxt->log_level = LOG_INFO;
    puts(cpp_17 ?"true":"false");
    void* handle = dlopen("./dll.so", RTLD_LAZY);
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

