/**
 * Part of WinLamb - Win32 API Lambda Library
 * https://github.com/rodrigocfd/winlamb
 * This library is released under the MIT License
 */

#pragma once
#include <string_view>
#include <vector>
#include <Windows.h>

namespace _wli {

inline static HHOOK globalMsgBoxHook;
inline static HWND  globalMsgBoxParent;

// Input filter follows same C# syntax:
// L"Text Files (*.txt)|*.txt|All Files (*.*)|*.*"
// Returns a single null-separated string.
[[nodiscard]]  inline std::vector<wchar_t> format_file_filter(std::wstring_view filterWithPipes)
{
	std::vector<wchar_t> multiZ(filterWithPipes.length() + 2, L'\0'); // two terminating nulls

	for (size_t i = 0; i < filterWithPipes.length(); ++i) {
		multiZ[i] = (filterWithPipes[i] != L'|') ? filterWithPipes[i] : L'\0';
	}
	return multiZ;
}

}//namespace _wli