#include "server/aggregations.h"
#include "server/table.h"
#include <chrono>


int main() {
    using namespace std;
    size_t n = 1'000'000;
    int a[n];
    for (int i = 0; i < n; ++i) {
        a[i] = rand();
    }
//    vector_type<int> t;
    //print(t);
    auto time = chrono::high_resolution_clock::now();
    double k = 0;
#pragma omp simd
    for (size_t i = 0; i < n; ++i)
        k += a[i];
    k/=(double)n;
    printf("%f\n", k);
    printf("%llu", (chrono::high_resolution_clock::now() - time).count());
    return (size_t)k;
}


