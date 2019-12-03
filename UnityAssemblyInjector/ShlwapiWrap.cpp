#define WIN32_LEAN_AND_MEAN 1
#include "Utils.h"
#include <windows.h>
#include <SDKDDKVer.h>
#include <array>

#if defined(INJECT_TO_UE4)
#pragma comment(linker, "/DLL")

HINSTANCE m_origModule;

template<typename T>
void GetExport(T*& funcPtr, const char* funcName)
{
	if (!m_origModule)
	{
		CHAR modulePath[MAX_PATH];

		GetSystemDirectoryA(modulePath, MAX_PATH);
		strcat_s(modulePath, "\\");
		strcat_s(modulePath, "shlwapi.dll");

		m_origModule = LoadLibraryA(modulePath);
	}

	if (funcPtr != nullptr)
	{
		return;
	}

	funcPtr = reinterpret_cast<T*>(GetProcAddress(m_origModule, funcName));
}

#define EXPORT(funcname, ...) \
    static decltype(funcname)* p; \
    GetExport(p, #funcname); \
    return p(__VA_ARGS__); \
    __pragma(comment(linker, "/EXPORT:" __FUNCTION__ "=" __FUNCDNAME__))

extern "C" BOOL WINAPI PathFileExistsW(LPCWSTR pszPath)
{
	EXPORT(PathFileExistsW, pszPath);
}

#endif