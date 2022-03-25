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
        std::this_thread::sleep_for(1us);
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
int _main()
{
    using string = std::string;
    using std::cout;
    using std::thread;
    void* handle = dlopen("dll.so", RTLD_LAZY);
    io::CSVReader<4> in("../test.csv");
    in.read_header(io::ignore_extra_column, "a", "b", "c", "d");
    int a, b, cc, d;
    string tp, tpdf, st;
    while (in.read_row(a, b, cc, d))
        cout << a<< ' ' << b <<' ' << cc <<' ' << d << '\n';

    //code_snippet c = reinterpret_cast<code_snippet>(dlsym(handle, "dlmain"));
    //printf("%d", c(0));
    //dlclose(handle);
    vector_type<double> t1{ 1.2, 3.4, .2, 1e-5, 1, 3, 5, 4, 5};
    vector_type<int> t2{ 2, 4, 3, 5, 0, 2, 6, 1, 2};
    printf("x: ");
    const auto& ret = t1 - t2;
    for (const auto& x : ret) {
        printf("%lf ", x);
    }
    puts("");

    Context* cxt = new Context();
    auto stocks = TableInfo<int, int>("stocks", 2);
    cxt->tables.insert(std::make_pair("stocks", &stocks));

    auto& stocks_0 = get<0>(stocks);
    auto& stocks_1 = get<1>(stocks);

    stocks_0.init();
    stocks_1.init();

    stocks_0.emplace_back(1);
    stocks_1.emplace_back(15);
    stocks_0.emplace_back(2);
    stocks_1.emplace_back(19);
    stocks_0.emplace_back(3);
    stocks_1.emplace_back(16);
    stocks_0.emplace_back(4);
    stocks_1.emplace_back(17);
    stocks_0.emplace_back(5);
    stocks_1.emplace_back(15);
    stocks_0.emplace_back(6);
    stocks_1.emplace_back(13);
    stocks_0.emplace_back(7);
    stocks_1.emplace_back(5);
    stocks_0.emplace_back(8);
    stocks_1.emplace_back(8);
    stocks_0.emplace_back(9);
    stocks_1.emplace_back(7);
    stocks_0.emplace_back(10);
    stocks_1.emplace_back(13);
    stocks_0.emplace_back(11);
    stocks_1.emplace_back(11);
    stocks_0.emplace_back(12);
    stocks_1.emplace_back(14);
    stocks_0.emplace_back(13);
    stocks_1.emplace_back(10);
    stocks_0.emplace_back(14);
    stocks_1.emplace_back(5);
    stocks_0.emplace_back(15);
    stocks_1.emplace_back(2);
    stocks_0.emplace_back(16);
    stocks_1.emplace_back(5);
    printf("%d\n", max(stocks_0 - mins(stocks_1)));
    return 0;
}

