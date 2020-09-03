/**
 * Part of WinLamb - Win32 API Lambda Library
 * https://github.com/rodrigocfd/winlamb
 * This library is released under the MIT License
 */

#pragma once
#include <string>
#include <Windows.h>

namespace _wli {

template<typename T>
[[nodiscard]] inline T format_raw_arg(T val) noexcept
{
	static_assert(!std::is_same<T, const char*>::value,
		"Non-wide char* being used on str::format(), str::to_wstring() can fix it.");
	static_assert(!std::is_same<T, std::string>::value,
		"Non-wide std::string being used on str::format(), str::to_wstring() can fix it.");
	return val;
}

[[nodiscard]] inline const wchar_t* format_raw_arg(const std::wstring& val) noexcept
{
	return val.c_str(); // so format() will also accept const wstring& in formatting list
}

[[nodiscard]] inline const wchar_t* format_raw_arg(std::wstring_view val) noexcept
{
	return val.data(); // so format() will also accept wstring_view in formatting list
}

template<typename ...argsT>
[[nodiscard]] inline std::wstring format_raw(std::wstring_view strFormat, const argsT&... args)
{
	// https://msdn.microsoft.com/en-us/magazine/dn913181.aspx
	// https://stackoverflow.com/a/514921/6923555
	size_t len = swprintf(nullptr, 0, strFormat.data(), format_raw_arg(args)...);
	std::wstring ret(len + 1, L'\0'); // room for terminating null
	swprintf(&ret[0], len + 1, strFormat.data(), format_raw_arg(args)...);
	ret.resize(len); // remove terminating null
	return ret;
}

[[nodiscard]] inline std::wstring get_window_text(HWND hWnd)
{
	std::wstring buf;
	int len = GetWindowTextLengthW(hWnd);
	if (len) {
		buf.resize(static_cast<size_t>(len) + 1, L'\0');
		GetWindowTextW(hWnd, &buf[0], len + 1);
		buf.resize(len);
	}
	return buf;
}

}//namespace _wli