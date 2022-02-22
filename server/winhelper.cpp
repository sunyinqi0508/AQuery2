#include "winhelper.h"
#include <Windows.h>


void* dlopen(const char* lib, int)
{
	return LoadLibraryA(lib);
}

void* dlsym(void* handle, const char* proc)
{
	return GetProcAddress(static_cast<HMODULE>(handle), proc);
}

int dlclose(void* handle)
{
	return FreeLibrary(static_cast<HMODULE>(handle));
}
