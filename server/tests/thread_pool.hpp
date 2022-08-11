#include "../threading.h"
#include <thread>
#include <cstdio>
#include <cstdlib>
using namespace std;

FILE *fp;
int testing_throughput(uint32_t n_jobs){
    printf("Threadpool througput test with %u jobs.\n", n_jobs);

    auto tp = ThreadPool(thread::hardware_concurrency());
    getchar();
    auto i = 0u;
    fp = fopen("tmp.tmp", "w");
    auto time = chrono::high_resolution_clock::now();
    while(i++ < n_jobs) tp.enqueue_task({ [](void* f) {fprintf(fp, "%d ", *(int*)f); free(f); }, new int(i) });
    puts("done dispatching.");
    while (tp.busy()) this_thread::sleep_for(1s);
    auto t = (chrono::high_resolution_clock::now() - time).count();
    printf("\nTr: %u, Ti: %lld \nThroughput: %lf transactions/ns\n", i, t, i/(double)(t));
    //this_thread::sleep_for(2s);
    return 0;
}

int testing_transaction(uint32_t n_burst, uint32_t n_batch, 
    uint32_t base_time, uint32_t var_time){
    printf("Threadpool transaction test: burst: %u, batch: %u, time: [%u, %u].\n"
         , n_burst, n_batch, base_time, var_time + base_time);
    
    auto tp = ThreadPool(thread::hardware_concurrency());
    getchar();
    auto i = 0u, j = 0u;
    auto time = chrono::high_resolution_clock::now();
    while(j++ < n_batch){
        i = 0u;
        while(i++ < n_burst) 
            tp.enqueue_task({ [](void* f) { printf( "%d ", *(int*)f); free(f); }, new int(i) });
        fflush(stdout);
        this_thread::sleep_for(chrono::microseconds(rand()%var_time + base_time));
    }
    puts("done dispatching.");
    while (tp.busy()) this_thread::sleep_for(1s);
    auto t = (chrono::high_resolution_clock::now() - time).count();
    printf("\nTr: %u, Ti: %lld \nThroughput: %lf transactions/ns\n", j*i, t, j*i/(double)(t));
    return 0;

}
