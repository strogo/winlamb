/**
 * Part of WinLamb - Win32 API Lambda Library
 * https://github.com/rodrigocfd/winlamb
 * This library is released under the MIT License
 */

#pragma once
#include <string>
#include <Windows.h>

namespace _wli {

[[nodiscard]] inline WORD parse_uint16_be(const BYTE* data) noexcept
{
	return (data[0] << 8) | data[1];
}

[[nodiscard]] inline WORD parse_uint16_le(const BYTE* data) noexcept
{
	return data[0] | (data[1] << 8);
}

[[nodiscard]] inline DWORD parse_uint32_be(const BYTE* data) noexcept
{
	return (data[0] << 24)
		| (data[1] << 16)
		| (data[2] << 8)
		| data[3];
}

[[nodiscard]] inline DWORD parse_uint32_le(const BYTE* data) noexcept
{
	return data[0]
		| (data[1] << 8)
		| (data[2] << 16)
		| (data[3] << 24);
}

// Parses binary ANSI string into Unicode wstring.
[[nodiscard]] inline std::wstring str_from_ansi(const BYTE* data, size_t sz)
{
	std::wstring ret;
	if (data && sz) {
		ret.resize(sz);
		for (size_t i = 0; i < sz; ++i) {
			if (!data[i]) { // found terminating null
				ret.resize(i);
				return ret;
			}
			ret[i] = static_cast<wchar_t>(data[i]); // raw conversion
		}
	}
	return ret; // data didn't have a terminating null
}

// Parses binary codepaged data into Unicode wstring with MultiByteToWideChar().
[[nodiscard]] inline std::wstring str_from_code_page(const BYTE* data, size_t sz, UINT codePage)
{
	std::wstring ret;
	if (data && sz) {
		int neededLen = MultiByteToWideChar(codePage, 0, reinterpret_cast<const char*>(data),
			static_cast<int>(sz), nullptr, 0);
		ret.resize(neededLen);
		MultiByteToWideChar(codePage, 0, reinterpret_cast<const char*>(data),
			static_cast<int>(sz), &ret[0], neededLen);
		ret.resize( lstrlenW(ret.c_str()) ); // trim_nulls()
	}
	return ret;
}

// Parses binary UTF-16 (BE or LE) data into Unicode string.
[[nodiscard]] inline std::wstring str_from_utf16(const BYTE* data, size_t sz, bool isBigEndian)
{
	if (sz % 2) {
		--sz; // we have an odd number of bytes, discard last one
	}

	std::wstring buf;
	buf.reserve(sz / 2);

	for (size_t i = 0; i < sz; i += 2) {
		buf.append(1, isBigEndian
			? parse_uint16_be(&data[i])
			: parse_uint16_le(&data[i]));
	}
	return buf;
}

}//namespace _wli