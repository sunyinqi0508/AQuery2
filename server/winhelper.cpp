#include "pch_msc.hpp"

#ifdef _WIN32
#include "winhelper.h"
#include <Windows.h>


void* dlopen(const char* lib, int)
{
	return LoadLibraryA(lib);
}

void* dlsym(void* handle, const char* proc)
{
	return reinterpret_cast<void*>(GetProcAddress(static_cast<HMODULE>(handle), proc));
}

int dlclose(void* handle)
{
	return FreeLibrary(static_cast<HMODULE>(handle));
}

int dlerror() {
    return GetLastError();
}

SharedMemory::SharedMemory(const char* fname)
{
    this->hFileMap = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, 2, fname);
    if (this->hFileMap)
        this->pData = MapViewOfFile(this->hFileMap, FILE_MAP_ALL_ACCESS, 0, 0, 2);
    else
        this->pData = NULL;
}

void SharedMemory::FreeMemoryMap()
{
    if (this->hFileMap) 
        if (this->pData)
            UnmapViewOfFile(this->pData);
        if (this->hFileMap)
            CloseHandle(this->hFileMap);
}

#ifndef __USE_STD_SEMAPHORE__
A_Semaphore::A_Semaphore(bool v = false) {
    native_handle = CreateSemaphore(NULL, v, 1, NULL);
}
void A_Semaphore::acquire() {
    WaitForSingleObject(native_handle, INFINITE);
}
void A_Semaphore::release() {
    ReleaseSemaphore(native_handle, 1, NULL);
}
A_Semaphore::~A_Semaphore() {
    CloseHandle(native_handle);
}
#endif

#endif
