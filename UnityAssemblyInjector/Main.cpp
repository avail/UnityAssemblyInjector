#define _CRT_SECURE_NO_WARNINGS 1

#include <Windows.h>
#include <map>
#include <vector>
#include <fstream>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <iterator>
#include <strsafe.h>
#include <filesystem>

#include "deps/include/MinHook.h"
#include "Utils.h"

#if _M_IX86
#pragma comment(lib, "deps/lib/MinHook-x86.lib")
#else
#pragma comment(lib, "deps/lib/MinHook-x64.lib")
#endif

#ifdef _DEBUG
#define DBGPRINT(kwszDebugFormatString, ...) _DBGPRINT(__FUNCTIONW__, __LINE__, kwszDebugFormatString, __VA_ARGS__)

VOID _DBGPRINT(LPCWSTR kwszFunction, INT iLineNumber, LPCWSTR kwszDebugFormatString, ...) \
{
	INT cbFormatString = 0;
	va_list args;
	PWCHAR wszDebugString = NULL;
	size_t st_Offset = 0;

	va_start(args, kwszDebugFormatString);

	cbFormatString = _scwprintf(L"[%s:%d] ", kwszFunction, iLineNumber) * sizeof(WCHAR);
	cbFormatString += _vscwprintf(kwszDebugFormatString, args) * sizeof(WCHAR) + 2;

	wszDebugString = (PWCHAR)_malloca(cbFormatString);

	StringCbPrintfW(wszDebugString, cbFormatString, L"[%s:%d] ", kwszFunction, iLineNumber);
	StringCbLengthW(wszDebugString, cbFormatString, &st_Offset);
	StringCbVPrintfW(&wszDebugString[st_Offset / sizeof(WCHAR)], cbFormatString - st_Offset, kwszDebugFormatString, args);

	OutputDebugStringW(wszDebugString);

	_freea(wszDebugString);
	va_end(args);
}
#else
#define DBGPRINT( kwszDebugFormatString, ... ) ;;
#endif

HMODULE monoHandle = NULL;

std::map<std::string, std::string> dllsToLoad;

typedef DWORD(*lp_mono_security_get_mode)();
typedef void(*lp_mono_security_set_mode)(DWORD mode);
typedef void*(*lp_mono_domain_get)();
typedef void*(*lp_mono_domain_assembly_open)(PVOID domain, PCHAR file);
typedef void*(*lp_mono_assembly_get_image)(PVOID assembly);
typedef void*(*lp_mono_class_from_name)(PVOID image, PCHAR namespacee, PCHAR name);
typedef void*(*lp_mono_class_get_method_from_name)(PVOID classs, PCHAR name, DWORD param_count);
typedef void*(*lp_mono_runtime_invoke)(PVOID method, PVOID instance, PVOID *params, PVOID exc);
typedef void*(*lp_mono_thread_attach)(PVOID domain);
typedef void*(*lp_mono_get_root_domain)();
typedef void*(*lp_mono_method_desc_search_in_image)(void*, void*);
typedef void*(*lp_mono_method_desc_free)(void*);
typedef void*(*lp_mono_method_desc_new)(const char*, bool);

lp_mono_security_get_mode mono_security_get_mode;
lp_mono_security_set_mode mono_security_set_mode;
lp_mono_domain_get mono_domain_get;
lp_mono_domain_assembly_open mono_domain_assembly_open;
lp_mono_assembly_get_image mono_assembly_get_image;
lp_mono_class_from_name mono_class_from_name;
lp_mono_class_get_method_from_name mono_class_get_method_from_name;
lp_mono_runtime_invoke mono_runtime_invoke;
lp_mono_thread_attach mono_thread_attach;
lp_mono_get_root_domain mono_get_root_domain;
lp_mono_method_desc_search_in_image mono_method_desc_search_in_image;
lp_mono_method_desc_free mono_method_desc_free;
lp_mono_method_desc_new mono_method_desc_new;

typedef void*(WINAPI *lp_mono_assembly_load_from_full)(void*, const char*, void*, bool);
lp_mono_assembly_load_from_full mono_assembly_load_from_full = NULL;

#define method_search(name, method) \
		description = mono_method_desc_new(name, 1); \
		method = mono_method_desc_search_in_image(description, scriptManagerImage); \
		mono_method_desc_free(description); \
		methodSearchSuccess = methodSearchSuccess && method != NULL

void* mono_assembly_load_from_full_hk(void* image, const char* fname, void* status, bool refonly)
{
	void* val = mono_assembly_load_from_full(image, fname, status, refonly);

	if (strstr(fname, "Assembly-CSharp.dll"))
	{
		DBGPRINT(L"Assembly-CSharp.dll is loading...");

		mono_security_get_mode = (lp_mono_security_get_mode)GetProcAddress(monoHandle, "mono_security_get_mode");
		mono_security_set_mode = (lp_mono_security_set_mode)GetProcAddress(monoHandle, "mono_security_set_mode");
		mono_domain_get = (lp_mono_domain_get)GetProcAddress(monoHandle, "mono_domain_get");
		mono_domain_assembly_open = (lp_mono_domain_assembly_open)GetProcAddress(monoHandle, "mono_domain_assembly_open");
		mono_assembly_get_image = (lp_mono_assembly_get_image)GetProcAddress(monoHandle, "mono_assembly_get_image");
		mono_class_from_name = (lp_mono_class_from_name)GetProcAddress(monoHandle, "mono_class_from_name");
		mono_class_get_method_from_name = (lp_mono_class_get_method_from_name)GetProcAddress(monoHandle, "mono_class_get_method_from_name");
		mono_runtime_invoke = (lp_mono_runtime_invoke)GetProcAddress(monoHandle, "mono_runtime_invoke");
		mono_thread_attach = (lp_mono_thread_attach)GetProcAddress(monoHandle, "mono_thread_attach");
		mono_get_root_domain = (lp_mono_get_root_domain)GetProcAddress(monoHandle, "mono_get_root_domain");
		mono_method_desc_search_in_image = (lp_mono_method_desc_search_in_image)GetProcAddress(monoHandle, "mono_method_desc_search_in_image");
		mono_method_desc_free = (lp_mono_method_desc_free)GetProcAddress(monoHandle, "mono_method_desc_free");
		mono_method_desc_new = (lp_mono_method_desc_new)GetProcAddress(monoHandle, "mono_method_desc_new");

		auto rootDomain = mono_get_root_domain();

		auto Start = [&](std::string name, std::string entryPoint)
		{
			auto scriptManagerAssembly = mono_domain_assembly_open(rootDomain, (PCHAR)(name.c_str()));

			if (!scriptManagerAssembly)
			{
				DBGPRINT(L"[%s] Failed to load.", ToWide(name).c_str());
				return;
			}

			auto scriptManagerImage = mono_assembly_get_image(scriptManagerAssembly);

			bool methodSearchSuccess = true;
			void* description;

			void* rtInitMethod;
			method_search(entryPoint.c_str(), rtInitMethod);

			if (!methodSearchSuccess)
			{
				DBGPRINT(L"[%s] Couldn't find entry point %s.", ToWide(name).c_str(), ToWide(entryPoint).c_str());

				return;
			}

			void* exc = nullptr;
			mono_runtime_invoke(rtInitMethod, nullptr, nullptr, &exc);

			if (exc)
			{
				DBGPRINT(L"[%s] Failed to invoke initialization method.", ToWide(name).c_str());
			}

			return;
		};

		for (auto& library : dllsToLoad)
		{
			Start(library.first, library.second);
		}
	}

	return val;
}

template<typename Out>
void split(const std::string &s, char delim, Out result) {
	std::stringstream ss(s);
	std::string item;

	while (std::getline(ss, item, delim)) 
	{
		*(result++) = item;
	}
}

std::vector<std::string> split(const std::string& s, char delim) {
	std::vector<std::string> elems;

	split(s, delim, std::back_inserter(elems));

	return elems;
}

std::vector<std::string> get_directories(const std::string& startDirectory)
{
	std::vector<std::string> ret;

	for (auto& path : std::filesystem::recursive_directory_iterator(startDirectory))
	{
		if (path.status().type() == std::filesystem::file_type::directory)
		{
			ret.push_back(path.path().string());
		}
	}

	return ret;
}

bool ends_with(std::string const& fullString, std::string const& ending)
{
	if (fullString.length() >= ending.length())
	{
		return (0 == fullString.compare(fullString.length() - ending.length(), ending.length(), ending));
	}
	else
	{
		return false;
	}
}

#include "Utils.h"

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
	if (ul_reason_for_call == DLL_PROCESS_ATTACH)
	{
		DBGPRINT(L"UnityAssemblyInjector attached");

		std::ifstream file("assemblies.txt");
		std::string str;

		while (std::getline(file, str))
		{
			std::vector<std::string> elems = split(str, '=');

			if (elems.size() == 2 && !elems[0].empty() && !elems[1].empty())
			{
				DBGPRINT(L"Loading assembly %s with init point %s", ToWide(elems[0]).c_str(), ToWide(elems[1]).c_str());

				dllsToLoad.insert(std::pair<std::string, std::string>(elems[0], elems[1]));
			}
		}

		std::vector<std::string> directories = get_directories(std::filesystem::current_path().string());

		std::string dataDirectory;

		for (auto& directory : directories)
		{
			if (ends_with(directory, "_Data"))
			{
				dataDirectory = directory;
			}
		}

		std::vector<std::string> searchPaths = {
			"Mono",
			"Mono\\EmbedRuntime",
			"MonoBleedingEdge\\EmbedRuntime"
		};

		std::vector<std::string> searchNames = {
			"mono.dll", // older mono builds
			"mono-2.0-bdwgc.dll", // unity gc mono builds
			"mono-2.0-sgen.dll", // oficial gc mono builds
			"mono-2.0-boehm.dll" // official mono builds with boehm's gc
		};

		std::string monoPath = "";
		std::string assemblyName = "";

		for (auto& path : searchPaths)
		{
			for (auto& name : searchNames)
			{
				std::string tryPath = va("%s\\%s", path.c_str(), name.c_str());

				if (std::filesystem::exists(tryPath))
				{
					monoPath = tryPath;
					assemblyName = name;
					break;
				}

				std::string tryPath2 = va("%s\\%s", dataDirectory.c_str(), tryPath.c_str());

				if (std::filesystem::exists(tryPath))
				{
					monoPath = tryPath2;
					assemblyName = name;
					break;
				}
			}

			if (monoPath != "")
			{
				break;
			}
		}

		if (monoPath == "")
		{
			DBGPRINT(L"Couldn't find mono.dll");
			return false;
		}

		monoHandle = LoadLibraryA(monoPath.c_str());

		if (!monoHandle)
		{
			DBGPRINT(L"Failed to load mono.dll");
			return false;
		}

		MH_Initialize();
		MH_CreateHookApi(ToWide(assemblyName).c_str(), "mono_assembly_load_from_full", mono_assembly_load_from_full_hk, (void**)&mono_assembly_load_from_full);
		MH_EnableHook(MH_ALL_HOOKS);
	}

	if (ul_reason_for_call == DLL_PROCESS_DETACH)
	{
		MH_DisableHook(MH_ALL_HOOKS);
	}

	return true;
}