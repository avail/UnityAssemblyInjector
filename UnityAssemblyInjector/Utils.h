#pragma once

#define _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS 1

#include <string>

const char* va(const char* string, ...);
const wchar_t* va(const wchar_t* string, ...);

std::string ToNarrow(const std::wstring& wide);
std::wstring ToWide(const std::string& narrow);