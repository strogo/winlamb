/**
* Part of WinLamb - Win32 API Lambda Library
* https://github.com/rodrigocfd/winlamb
* This library is released under the MIT License
*/

#pragma once
#include <stdexcept>
#include <string>
#include <system_error>
#include <Windows.h>
#include <ShlObj.h>

namespace _wli {

[[nodiscard]] inline std::wstring sys_path_shell(int clsId)
{
	wchar_t buf[MAX_PATH + 1]{};
	if FAILED(SHGetFolderPathW(nullptr, clsId, nullptr, 0, buf)) { // won't have trailing backslash
		throw std::runtime_error("SHGetFolderPath failed.");
	}
	return {buf};
}

[[nodiscard]] inline std::wstring sys_path_temp()
{
	wchar_t buf[MAX_PATH + 1]{};
	if (!GetTempPathW(ARRAYSIZE(buf), buf)) { // will have trailing backslash here
		throw std::system_error(GetLastError(), std::system_category(),
			"GetTempPath failed.");
	}
	std::wstring ret{buf};
	ret.resize(ret.length() - 1); // remove trailing backslash
	return ret;
}

}//namespace _wli