#define WIN32_LEAN_AND_MEAN 1
#include "Utils.h"
#include <windows.h>
#include <SDKDDKVer.h>
#include <array>

#if not defined(INJECT_TO_UE4)
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
		strcat_s(modulePath, "version.dll");

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


extern "C" BOOL WINAPI GetFileVersionInfoA(LPCSTR lptstrFilename, DWORD dwHandle, DWORD dwLen, LPVOID lpData) 
{
	EXPORT(GetFileVersionInfoA, lptstrFilename, dwHandle, dwLen, lpData);
}

extern "C" int WINAPI GetFileVersionInfoByHandle(int hMem, LPCWSTR lpFileName, int v2, int v3) 
{
	EXPORT(GetFileVersionInfoByHandle, hMem, lpFileName, v2, v3);
}

extern "C" BOOL WINAPI  GetFileVersionInfoExA(DWORD dwFlags, LPCSTR lpwstrFilename, DWORD dwHandle, DWORD dwLen, LPVOID lpData) 
{
	EXPORT(GetFileVersionInfoExA, dwFlags, lpwstrFilename, dwHandle, dwLen, lpData);
}

extern "C" BOOL WINAPI GetFileVersionInfoExW(DWORD dwFlags, LPCWSTR lpwstrFilename, DWORD dwHandle, DWORD dwLen, LPVOID lpData) 
{
	EXPORT(GetFileVersionInfoExW, dwFlags, lpwstrFilename, dwHandle, dwLen, lpData);
}

extern "C" DWORD WINAPI GetFileVersionInfoSizeA(LPCSTR lptstrFilename, LPDWORD lpdwHandle) 
{
	EXPORT(GetFileVersionInfoSizeA, lptstrFilename, lpdwHandle);
}

extern "C" DWORD WINAPI GetFileVersionInfoSizeExA(DWORD dwFlags, LPCSTR lpwstrFilename, LPDWORD lpdwHandle) 
{
	EXPORT(GetFileVersionInfoSizeExA, dwFlags, lpwstrFilename, lpdwHandle);
}

extern "C" DWORD WINAPI GetFileVersionInfoSizeExW(DWORD dwFlags, LPCWSTR lpwstrFilename, LPDWORD lpdwHandle) 
{
	EXPORT(GetFileVersionInfoSizeExW, dwFlags, lpwstrFilename, lpdwHandle);
}

extern "C" DWORD WINAPI GetFileVersionInfoSizeW(LPCWSTR lptstrFilename, LPDWORD lpdwHandle) 
{
	EXPORT(GetFileVersionInfoSizeW, lptstrFilename, lpdwHandle);
}

extern "C" BOOL WINAPI GetFileVersionInfoW(LPCWSTR lptstrFilename, DWORD dwHandle, DWORD dwLen, LPVOID lpData) 
{
	EXPORT(GetFileVersionInfoW, lptstrFilename, dwHandle, dwLen, lpData);
}

extern "C" DWORD WINAPI VerFindFileA(DWORD uFlags, LPCSTR szFileName, LPCSTR szWinDir, LPCSTR szAppDir, LPSTR szCurDir, PUINT lpuCurDirLen, LPSTR szDestDir, PUINT lpuDestDirLen) 
{
	EXPORT(VerFindFileA, uFlags, szFileName, szWinDir, szAppDir, szCurDir, lpuCurDirLen, szDestDir, lpuDestDirLen);
}

extern "C" DWORD WINAPI VerFindFileW(DWORD uFlags, LPCWSTR szFileName, LPCWSTR szWinDir, LPCWSTR szAppDir, LPWSTR szCurDir, PUINT lpuCurDirLen, LPWSTR szDestDir, PUINT lpuDestDirLen) 
{
	EXPORT(VerFindFileW, uFlags, szFileName, szWinDir, szAppDir, szCurDir, lpuCurDirLen, szDestDir, lpuDestDirLen);
}

extern "C" DWORD WINAPI VerInstallFileA(DWORD uFlags, LPCSTR szSrcFileName, LPCSTR szDestFileName, LPCSTR szSrcDir, LPCSTR szDestDir, LPCSTR szCurDir, LPSTR szTmpFile, PUINT lpuTmpFileLen) 
{
	EXPORT(VerInstallFileA, uFlags, szSrcFileName, szDestFileName, szSrcDir, szDestDir, szCurDir, szTmpFile, lpuTmpFileLen);
}

extern "C" DWORD WINAPI VerInstallFileW(DWORD uFlags, LPCWSTR szSrcFileName, LPCWSTR szDestFileName, LPCWSTR szSrcDir, LPCWSTR szDestDir, LPCWSTR szCurDir, LPWSTR szTmpFile, PUINT lpuTmpFileLen) 
{
	EXPORT(VerInstallFileW, uFlags, szSrcFileName, szDestFileName, szSrcDir, szDestDir, szCurDir, szTmpFile, lpuTmpFileLen);
}

extern "C" DWORD WINAPI VerLanguageNameA(DWORD wLang, LPSTR szLang, DWORD cchLang) 
{
	EXPORT(VerLanguageNameA, wLang, szLang, cchLang);
}

extern "C" DWORD WINAPI VerLanguageNameW(DWORD wLang, LPWSTR szLang, DWORD cchLang) 
{
	EXPORT(VerLanguageNameW, wLang, szLang, cchLang);
}

extern "C" BOOL WINAPI VerQueryValueA(LPCVOID pBlock, LPCSTR lpSubBlock, LPVOID * lplpBuffer, PUINT puLen) 
{
	EXPORT(VerQueryValueA, pBlock, lpSubBlock, lplpBuffer, puLen);
}

extern "C" BOOL WINAPI VerQueryValueW(LPCVOID pBlock, LPCWSTR lpSubBlock, LPVOID * lplpBuffer, PUINT puLen) 
{
	EXPORT(VerQueryValueW, pBlock, lpSubBlock, lplpBuffer, puLen);
}
#endif