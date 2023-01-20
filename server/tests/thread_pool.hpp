#pragma once

#include "../threading.h"
#include <thread>
#include <cstdio>
#include <cstdlib>
using namespace std;

FILE *fp;
long long testing_throughput(uint32_t n_jobs, bool prompt = true){
    printf("Threadpool througput test with %u jobs. Press any key to start.\n", n_jobs);

    auto tp = ThreadPool(thread::hardware_concurrency());
    getchar();
    auto i = 0u;
    fp = fopen("tmp.tmp", "wb");
    auto time = chrono::high_resolution_clock::now();
    while(i++ < n_jobs) tp.enqueue_task({ [](void* f) {fprintf(fp, "%d ", *(int*)f); free(f); }, new int(i) });
    puts("done dispatching.");
    while (tp.busy()) this_thread::sleep_for(1s);
    auto t = (chrono::high_resolution_clock::now() - time).count();
    printf("\nTr: %u, Ti: %lld \nThroughput: %lf transactions/ns\n", i, t, i/(double)(t));
    //this_thread::sleep_for(2s);
    fclose(fp);
    return t;
}

long long testing_transaction(uint32_t n_burst, uint32_t n_batch, 
    uint32_t base_time, uint32_t var_time, bool prompt = true, FILE* _fp = stdout){
    printf("Threadpool transaction test: burst: %u, batch: %u, time: [%u, %u].\n"
         , n_burst, n_batch, base_time, var_time + base_time);
    if (prompt) {
        puts("Press any key to start.");
        getchar();
    }
    auto tp = ThreadPool(thread::hardware_concurrency());
    fp = _fp;
    auto i = 0u, j = 0u;
    auto time = chrono::high_resolution_clock::now();
    while(j++ < n_batch){
        i = 0u;
        while(i++ < n_burst) 
            tp.enqueue_task({ [](void* f) { fprintf(fp, "%d ", *(int*)f); free(f); }, new int(j) });
        fflush(stdout);
        this_thread::sleep_for(chrono::microseconds(rand()%var_time + base_time));
    }
    puts("done dispatching.");
    while (tp.busy()) this_thread::sleep_for(1s);
    auto t = (chrono::high_resolution_clock::now() - time).count();
    printf("\nTr: %u, Ti: %lld \nThroughput: %lf transactions/ns\n", j*i, t, j*i/(double)(t));
    return t;

}

long long testing_destruction(bool prompt = true){
    fp = fopen("tmp.tmp", "wb");
    if (prompt) {
        puts("Press any key to start.");
        getchar();
    }
    auto time = chrono::high_resolution_clock::now();
    for(int i = 0; i < 8; ++i)
        testing_transaction(0xfff, 0xff, 400, 100, false, fp);
    for(int i = 0; i < 64; ++i)
        testing_transaction(0xff, 0xf, 60, 20, false, fp);
    for(int i = 0; i < 1024; ++i) {
        auto tp = new ThreadPool(256);
        delete tp; 
    }
    return 0;
    auto t = (chrono::high_resolution_clock::now() - time).count();
    fclose(fp);
    return t;
}
