#include <iostream>
#include "libaquery.h"
#ifdef _WIN32
#include "winhelper.h"
#else 
#include <dlfcn.h>
#endif
typedef int (*code_snippet)(void*);
int main()
{
    void* handle = dlopen("dll.so", RTLD_LAZY);
    code_snippet c = static_cast<code_snippet>(dlsym(handle, "dlmain"));
    printf("%d", c(0));
    dlclose(handle);
    vector_type<double> t1{ 1.2, 3.4, .2, 1e-5, 1, 3, 5, 4, 5};
    vector_type<int> t2{ 2, 4, 3, 5, 0, 2, 6, 1, 2};
    printf("x: ");
    const auto& ret = t1 - t2;
    for (const auto& x : ret) {
        printf("%lf ", x);
    }
    puts("");
    return 0;
}

