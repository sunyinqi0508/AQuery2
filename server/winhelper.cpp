#include "pch.hpp"

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
#endif
