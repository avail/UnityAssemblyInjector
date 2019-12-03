#pragma once

#define _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS 1

#include <string>
#include <vector>
#include <sstream>

const char* va(const char* string, ...);
const wchar_t* va(const wchar_t* string, ...);

std::string ToNarrow(const std::wstring& wide);
std::wstring ToWide(const std::string& narrow);

template<typename Out>
void split(const std::string& s, char delim, Out result);

std::vector<std::string> split(const std::string& s, char delim);
std::vector<std::string> get_directories(const std::string& startDirectory);
bool ends_with(std::string const& fullString, std::string const& ending);

//#define INJECT_TO_UE4 1
//#define INJECT_TO_GAMEASSEMBLY 1