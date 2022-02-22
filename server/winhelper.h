#ifndef _WINHELPER_H
#define _WINHELPER_H
#define RTLD_LAZY 1
void* dlopen(const char*, int);
void* dlsym(void*, const char*);
int dlclose(void*);
#endif
