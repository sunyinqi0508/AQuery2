#ifndef _WINHELPER_H
#define _WINHELPER_H

#ifdef _WIN32
static constexpr int RTLD_LAZY = 1, RTLD_NOW = 1;
void* dlopen(const char*, int);
void* dlsym(void*, const char*);
int dlclose(void*);
int dlerror();

struct SharedMemory
{
    void* hFileMap;
    void* pData;
    SharedMemory(const char*);
    void FreeMemoryMap();
};
class A_Semaphore {
private:
	void* native_handle;
public:
	A_Semaphore(bool);
	void acquire();
	void release();
	~A_Semaphore();
};
#endif // WIN32

#endif // WINHELPER
