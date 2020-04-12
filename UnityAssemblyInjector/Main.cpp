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

#define INJECT_TO_GAMEASSEMBLY

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

std::map<std::string, std::string> dllsToLoad;


HMODULE monoHandle = NULL;

typedef void*(WINAPI* lp_mono_assembly_load_from_full)(void*, const char*, void*, bool);
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

typedef void*(*lp_mono_object_get_class)(void*);
typedef void*(*lp_mono_object_to_string)(void*, void*);
typedef void*(*lp_mono_class_get_property_from_name)(void*, const char*);
typedef void*(*lp_mono_property_get_get_method)(void*);
typedef const char*(*lp_mono_string_to_utf8)(void*);

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

lp_mono_object_get_class mono_object_get_class;
lp_mono_object_to_string mono_object_to_string;
lp_mono_class_get_property_from_name mono_class_get_property_from_name;
lp_mono_property_get_get_method mono_property_get_get_method;
lp_mono_string_to_utf8 mono_string_to_utf8;
lp_mono_assembly_load_from_full mono_assembly_load_from_full;

typedef void* (*lp_il2cpp_init)(const char* name);
typedef void(*lp_il2cpp_thread_attach)(void* domain);
typedef void* (*lp_il2cpp_domain_assembly_open)(void* domain, const char* name);
typedef void* (*lp_il2cpp_assembly_get_image)(void* assembly);
typedef void* (*lp_il2cpp_class_from_name)(void* image, const char* name_space, const char* name);
typedef void* (*lp_il2cpp_class_get_method_from_name)(void* _class, const char* name, int param_count);
typedef void* (*lp_il2cpp_runtime_invoke)(void* method, void* obj, void** params, void** exc);

lp_il2cpp_init il2cpp_init;
lp_il2cpp_thread_attach il2cpp_thread_attach;
lp_il2cpp_domain_assembly_open il2cpp_domain_assembly_open;
lp_il2cpp_assembly_get_image il2cpp_assembly_get_image;
lp_il2cpp_class_from_name il2cpp_class_from_name;
lp_il2cpp_class_get_method_from_name il2cpp_class_get_method_from_name;
lp_il2cpp_runtime_invoke il2cpp_runtime_invoke;


#define method_search(name, method) \
		description = mono_method_desc_new(name, 1); \
		method = mono_method_desc_search_in_image(description, scriptManagerImage); \
		mono_method_desc_free(description); \
		methodSearchSuccess = methodSearchSuccess && method != NULL


static void OutputExceptionDetails(void* exc)
{
#if not defined (INJECT_TO_GAMEASSEMBLY)
	void* eclass = mono_object_get_class(exc);

	if (eclass)
	{
		//void* toStringExc = nullptr;
		//void* msg = mono_object_to_string(exc, &toStringExc);

		void* prop = mono_class_get_property_from_name(eclass, "StackTrace");
		void* getter = mono_property_get_get_method(prop);
		void* msg2 = mono_runtime_invoke(getter, exc, NULL, NULL);

		//if (toStringExc)
		//{
			prop = mono_class_get_property_from_name(eclass, "Message");
			getter = mono_property_get_get_method(prop);
			void* msg = mono_runtime_invoke(getter, exc, NULL, NULL);
		//}

		DBGPRINT(L"Unhandled exception in Mono script environment: %s\n%s", ToWide(mono_string_to_utf8(msg)).c_str(), ToWide(mono_string_to_utf8(msg2)).c_str());
	}
#endif
}

#if not defined(INJECT_TO_GAMEASSEMBLY)

bool readyForInvoke = false;
bool wasReady = false;

void* mono_runtime_invoke_hk(PVOID method, PVOID instance, PVOID* params, PVOID exc)
{
	if (readyForInvoke && !wasReady)
	{
		readyForInvoke = false;
		wasReady = true;

		auto rootDomain = mono_get_root_domain();

		auto Exec = [&](std::string name, std::string entryPoint)
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
				OutputExceptionDetails(exc);
				DBGPRINT(L"[%s] Failed to invoke initialization method.", ToWide(name).c_str());
			}

			return;
		};

		for (auto& library : dllsToLoad)
		{
			Exec(library.first, library.second);
		}
	}

	return mono_runtime_invoke(method, instance, params, exc);
}

void* mono_assembly_load_from_full_hk(void* image, const char* fname, void* status, bool refonly)
{
	static bool isSet = false;


	if (!isSet)
	{
		isSet = true;

		mono_security_get_mode = (lp_mono_security_get_mode)GetProcAddress(monoHandle, "mono_security_get_mode");
		mono_security_set_mode = (lp_mono_security_set_mode)GetProcAddress(monoHandle, "mono_security_set_mode");
		mono_domain_get = (lp_mono_domain_get)GetProcAddress(monoHandle, "mono_domain_get");
		mono_domain_assembly_open = (lp_mono_domain_assembly_open)GetProcAddress(monoHandle, "mono_domain_assembly_open");
		mono_assembly_get_image = (lp_mono_assembly_get_image)GetProcAddress(monoHandle, "mono_assembly_get_image");
		mono_class_from_name = (lp_mono_class_from_name)GetProcAddress(monoHandle, "mono_class_from_name");
		mono_class_get_method_from_name = (lp_mono_class_get_method_from_name)GetProcAddress(monoHandle, "mono_class_get_method_from_name");
		//mono_runtime_invoke = (lp_mono_runtime_invoke)GetProcAddress(monoHandle, "mono_runtime_invoke");
		mono_thread_attach = (lp_mono_thread_attach)GetProcAddress(monoHandle, "mono_thread_attach");
		mono_get_root_domain = (lp_mono_get_root_domain)GetProcAddress(monoHandle, "mono_get_root_domain");
		mono_method_desc_search_in_image = (lp_mono_method_desc_search_in_image)GetProcAddress(monoHandle, "mono_method_desc_search_in_image");
		mono_method_desc_free = (lp_mono_method_desc_free)GetProcAddress(monoHandle, "mono_method_desc_free");
		mono_method_desc_new = (lp_mono_method_desc_new)GetProcAddress(monoHandle, "mono_method_desc_new");

		mono_object_get_class = (lp_mono_object_get_class)GetProcAddress(monoHandle, "mono_object_get_class");
		mono_object_to_string = (lp_mono_object_to_string)GetProcAddress(monoHandle, "mono_object_to_string");
		mono_class_get_property_from_name = (lp_mono_class_get_property_from_name)GetProcAddress(monoHandle, "mono_class_get_property_from_name");
		mono_property_get_get_method = (lp_mono_property_get_get_method)GetProcAddress(monoHandle, "mono_property_get_get_method");
		mono_string_to_utf8 = (lp_mono_string_to_utf8)GetProcAddress(monoHandle, "mono_string_to_utf8");
	}


	DBGPRINT(L"Assembly %s is loading", ToWide(fname).c_str());

	if (strstr(fname, "UnityEngine.dll"))
	{
		readyForInvoke = true;
	}

	DBGPRINT(L"%p %p", mono_domain_get(), mono_get_root_domain());

	return mono_assembly_load_from_full(image, fname, status, refonly);;
}
#else

#include <metahost.h>
#include <roapi.h>

MIDL_INTERFACE("712AB73F-2C22-4807-AD7E-F501D7B72C2D")
ICLRRuntimeHost2 : public ICLRRuntimeHost
{
public:
	virtual HRESULT STDMETHODCALLTYPE CreateAppDomainWithManager(
		/* [in] */ LPCWSTR wszFriendlyName,
		/* [in] */ DWORD dwFlags,
		/* [in] */ LPCWSTR wszAppDomainManagerAssemblyName,
		/* [in] */ LPCWSTR wszAppDomainManagerTypeName,
		/* [in] */ int nProperties,
		/* [in] */ LPCWSTR * pPropertyNames,
		/* [in] */ LPCWSTR * pPropertyValues,
		/* [out] */ DWORD * pAppDomainID) = 0;

	virtual HRESULT STDMETHODCALLTYPE CreateDelegate(
		/* [in] */ DWORD appDomainID,
		/* [in] */ LPCWSTR wszAssemblyName,
		/* [in] */ LPCWSTR wszClassName,
		/* [in] */ LPCWSTR wszMethodName,
		/* [out] */ INT_PTR* fnPtr) = 0;

	virtual HRESULT STDMETHODCALLTYPE Authenticate(
		/* [in] */ ULONGLONG authKey) = 0;

	virtual HRESULT STDMETHODCALLTYPE RegisterMacEHPort(void) = 0;

	virtual HRESULT STDMETHODCALLTYPE SetStartupFlags(
		/* [in] */ STARTUP_FLAGS dwFlags) = 0;

	virtual HRESULT STDMETHODCALLTYPE DllGetActivationFactory(
		/* [in] */ DWORD appDomainID,
		/* [in] */ LPCWSTR wszTypeName,
		/* [out] */ IActivationFactory** factory) = 0;

	virtual HRESULT STDMETHODCALLTYPE ExecuteAssembly(
		/* [in] */ DWORD dwAppDomainId,
		/* [in] */ LPCWSTR pwzAssemblyPath,
		/* [in] */ int argc,
		/* [in] */ LPCWSTR* argv,
		/* [out] */ DWORD* pReturnValue) = 0;

};

enum APPDOMAIN_SECURITY_FLAGS2
{
	//APPDOMAIN_SECURITY_DEFAULT = 0,
	//APPDOMAIN_SECURITY_SANDBOXED = 0x1,
	//APPDOMAIN_SECURITY_FORBID_CROSSAD_REVERSE_PINVOKE = 0x2,
	APPDOMAIN_IGNORE_UNHANDLED_EXCEPTIONS = 0x4,
	//APPDOMAIN_FORCE_TRIVIAL_WAIT_OPERATIONS = 0x8,
	APPDOMAIN_ENABLE_PINVOKE_AND_CLASSIC_COMINTEROP = 0x10,
	APPDOMAIN_ENABLE_PLATFORM_SPECIFIC_APPS = 0x40,
	APPDOMAIN_ENABLE_ASSEMBLY_LOADFILE = 0x80,
	APPDOMAIN_DISABLE_TRANSPARENCY_ENFORCEMENT = 0x100
};


#include <mscoree.h>
#pragma comment(lib, "mscoree.lib")
ICLRMetaHost* metaHost = NULL;
ICLRRuntimeInfo* runtimeInfo = NULL;
ICLRRuntimeHost2* runtimeHost = NULL;

#include <assert.h>

typedef HRESULT(STDAPICALLTYPE* FnGetCLRRuntimeHost)(REFIID riid, IUnknown** pUnk);

#define PROXY_HOOK(addr, funcName) \
	MH_CreateHook(addr, Hook_##funcName, (void**)&orig_##funcName);

#define SETUP_PROXY(funcName) \
	void (*orig_##funcName)(void*, void*, void*, void*) = nullptr; \
\
	void (*cust_##funcName)(); \
\
	extern "C" __declspec(dllexport) void Set_##funcName(void(*action)()) \
	{ \
		cust_##funcName = action; \
	} \
\
	void Hook_##funcName(void* a, void* b, void* c, void* d) \
	{ \
		if (cust_##funcName) cust_##funcName(); \
		return orig_##funcName(a, b, c, d); \
	}

SETUP_PROXY(Destroy);
SETUP_PROXY(Start);
SETUP_PROXY(Update);
//SETUP_PROXY(OnGUI);

void* il2cpp_init_hk(const char* name) 
{
	void* domain = il2cpp_init(name);

	DBGPRINT(L"il2cpp_init_hk! domain is %x", domain);

	//CLRCreateInstance(CLSID_CLRMetaHost, IID_ICLRMetaHost, (LPVOID*)&metaHost);
	//metaHost->GetRuntime(L"v4.0.30319", IID_ICLRRuntimeInfo, (LPVOID*)&runtimeInfo);
	//runtimeInfo->GetInterface(CLSID_CLRRuntimeHost, IID_ICLRRuntimeHost, (LPVOID*)&runtimeHost);

	AddDllDirectory(L"C:\\Program Files\\dotnet\\shared\\Microsoft.NETCore.App\\3.1.3\\");
	auto lib = LoadLibraryW(L"C:\\Program Files\\dotnet\\shared\\Microsoft.NETCore.App\\3.1.3\\coreclr.dll");
	assert(lib);

	auto getClrRuntimeHost = (FnGetCLRRuntimeHost)GetProcAddress(lib, "GetCLRRuntimeHost");
	HRESULT hr = getClrRuntimeHost(__uuidof(ICLRRuntimeHost2), (IUnknown**)&runtimeHost);
	assert(SUCCEEDED(hr));

	runtimeHost->Start();

	wchar_t asd[MAX_PATH];
	GetCurrentDirectoryW(std::size(asd), asd);
	StringCchCatW(asd, std::size(asd), L"\\NekoClient");

	const wchar_t* appPath = va(L"C:\\Program Files\\dotnet\\shared\\Microsoft.NETCore.App\\3.1.3\\;%s", asd);

	const wchar_t* property_keys[] = {
		//L"TRUSTED_PLATFORM_ASSEMBLIES",
		L"APP_PATHS",
		//L"APP_NI_PATHS",
		//L"NATIVE_DLL_SEARCH_DIRECTORIES",
		//L"APP_LOCAL_WINMETADATA"
	};
	const wchar_t* property_values[] = {
		// TRUSTED_PLATFORM_ASSEMBLIES
		//tpaList,
		// APP_PATHS
		appPath,
		// APP_NI_PATHS
		//appNiPath,
		// NATIVE_DLL_SEARCH_DIRECTORIES
		//nativeDllSearchDirs,
		// APP_LOCAL_WINMETADATA
		//appLocalWinmetadata
	};

	DWORD domainId;

	runtimeHost->CreateAppDomainWithManager(L"HELLODOMAIN",
		APPDOMAIN_ENABLE_PLATFORM_SPECIFIC_APPS |
		APPDOMAIN_ENABLE_PINVOKE_AND_CLASSIC_COMINTEROP |
		APPDOMAIN_DISABLE_TRANSPARENCY_ENFORCEMENT,
		NULL,
		NULL,
		sizeof(property_keys) / sizeof(wchar_t*),
		property_keys,
		property_values,
		&domainId);

	auto Start = [&](std::string name, std::string entryPoint)
	{
		DWORD retval;

		auto entry = split(entryPoint, ':');

		DBGPRINT(L"Attempting load of assembly %s, path %s, init func %s", ToWide(name).c_str(), ToWide(entry[0]).c_str(), ToWide(entry[1]).c_str());
		auto res = runtimeHost->ExecuteInDefaultAppDomain(ToWide(name).c_str(), ToWide(entry[0]).c_str(), ToWide(entry[1]).c_str(), GetCommandLineW(), &retval);

		if (res != 0)
		{
			DBGPRINT(L"HRESULT WAS %d", res);
		}
	};

	for (auto& library : dllsToLoad)
	{
		Start(library.first, library.second);
	}

	return domain;
}
#endif

#include "Hooking.Patterns.h"

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
	if (ul_reason_for_call == DLL_PROCESS_ATTACH)
	{
		char moduleName[MAX_PATH];
		GetModuleFileNameA(GetModuleHandle(NULL), moduleName, sizeof(moduleName));

		if (strstr(moduleName, "UnityCrashHandler") != 0)
		{
			DBGPRINT(L"not attaching to crash handler");
			return -1;
		}

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

#if not defined(INJECT_TO_GAMEASSEMBLY)
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

				if (std::filesystem::exists(tryPath2))
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
#endif

		MH_Initialize();
#if not defined (INJECT_TO_GAMEASSEMBLY)
		MH_CreateHookApi(ToWide(assemblyName).c_str(), "mono_assembly_load_from_full", mono_assembly_load_from_full_hk, (void**)&mono_assembly_load_from_full);
		MH_CreateHookApi(ToWide(assemblyName).c_str(), "mono_runtime_invoke", mono_runtime_invoke_hk, (void**)&mono_runtime_invoke);
#else
		auto gameAssembly = LoadLibraryA("GameAssembly.dll");
		auto unityPlayerAssembly = LoadLibraryA("UnityPlayer.dll");

		MH_CreateHookApi(L"GameAssembly.dll", "il2cpp_init", il2cpp_init_hk, (void**)&il2cpp_init);
		
		//PROXY_HOOK(hook::module_pattern(unityPlayerAssembly, "40 53 48 83 ec 20 48 8b ? ? ? ? ? 48 8b d9 8b ca 48 33").count(1).get(0).get<char*>(), Destroy);
		//PROXY_HOOK(hook::module_pattern(unityPlayerAssembly, "40 53 48 83 ec 20 80 b9 25 01 00 00 00").count(1).get(0).get<char*>(), Start);
		//PROXY_HOOK(hook::module_pattern(unityPlayerAssembly, "48 89 5c 24 10 57 48 83 ec 20 48 8b 81 f0 00 00 00 8b fa").count(1).get(0).get<char*>(), Update);
		//PROXY_HOOK(hook::module_pattern(unityPlayerAssembly, "c7 41 40 00 00 00 00 48 c7 41 04 ff ff ff ff").count(1).get(0).get<char*>(), OnGUI);
		PROXY_HOOK((char*)gameAssembly + 0x1EC0780, Destroy); // atexit: 40 53 48 83 ec 20 48 8b ? ? ? ? ? 48 8b d9 8b ca 48 33
		PROXY_HOOK((char*)gameAssembly + 0x1EC0960, Start); // Start: 40 53 48 83 ec 20 80 b9 25 01 00 00 00
		PROXY_HOOK((char*)gameAssembly + 0x1ECE080, Update); // Update: 48 89 5c 24 10 57 48 83 ec 20 48 8b 81 f0 00 00 00 8b fa
		//PROXY_HOOK((char*)gameAssembly + 0x1EC0590, OnGUI); // BeginOnGUI (orig first then proxy):  c7 41 40 00 00 00 00 48 c7 41 04 ff ff ff ff*/
#endif
		MH_EnableHook(MH_ALL_HOOKS);

	}

	if (ul_reason_for_call == DLL_PROCESS_DETACH)
	{
#if not defined (INJECT_TO_UE4)
		MH_DisableHook(MH_ALL_HOOKS);
#endif
	}

	return true;
}