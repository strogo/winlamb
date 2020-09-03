/**
 * Part of WinLamb - Win32 API Lambda Library
 * https://github.com/rodrigocfd/winlamb
 * This library is released under the MIT License
 */

#pragma once
#include <stdexcept>
#include <string_view>
#include <vector>
#include <Windows.h>
#include "internals/bin_aux.h"

// Utilities for binary/string conversions.
namespace wl::bin {

// Possible string encodings.
enum class encoding { UNKNOWN, ASCII, WIN1252, UTF8, UTF16BE, UTF16LE, UTF32BE, UTF32LE, SCSU, BOCU1 };

// Encoding information of a string.
struct encoding_info final {
	encoding encType = encoding::UNKNOWN;
	size_t   bomSize = 0;
};

// Guesses the encoding of binary string data.
[[nodiscard]] inline encoding_info guess_encoding(const BYTE* data, size_t sz) noexcept
{
	auto match = [&](const BYTE* pBom, int szBom) noexcept -> bool {
		return (sz >= static_cast<size_t>(szBom)) &&
			!memcmp(data, pBom, sizeof(BYTE) * szBom);
	};

	// https://en.wikipedia.org/wiki/Byte_order_mark

	BYTE utf8[] = {0xEF, 0xBB, 0xBF}; // UTF-8 BOM
	if (match(utf8, 3)) return {encoding::UTF8, 3}; // BOM size in bytes

	BYTE utf16be[] = {0xFE, 0xFF};
	if (match(utf16be, 2)) return {encoding::UTF16BE, 2};

	BYTE utf16le[] = {0xFF, 0xFE};
	if (match(utf16le, 2)) return {encoding::UTF16LE, 2};

	BYTE utf32be[] = {0x00, 0x00, 0xFE, 0xFF};
	if (match(utf32be, 4)) return {encoding::UTF32BE, 4};

	BYTE utf32le[] = {0xFF, 0xFE, 0x00, 0x00};
	if (match(utf32le, 4)) return {encoding::UTF32LE, 4};

	BYTE scsu[] = {0x0E, 0xFE, 0xFF};
	if (match(scsu, 3)) return {encoding::SCSU, 3};

	BYTE bocu1[] = {0xFB, 0xEE, 0x28};
	if (match(bocu1, 3)) return {encoding::BOCU1, 3};

	// No BOM found, guess UTF-8 without BOM, or Windows-1252 (superset of ISO-8859-1).
	bool canBeWin1252 = false;
	for (size_t i = 0; i < sz; ++i) {
		if (data[i] > 0x7F) { // 127
			canBeWin1252 = true;
			if (i <= sz - 2 && (
				(data[i] == 0xC2 && (data[i+1] >= 0xA1 && data[i+1] <= 0xBF)) || // http://www.utf8-chartable.de
				(data[i] == 0xC3 && (data[i+1] >= 0x80 && data[i+1] <= 0xBF)) ))
			{
				return {encoding::UTF8, 0}; // UTF-8 without BOM
			}
		}
	}
	return {(canBeWin1252 ? encoding::WIN1252 : encoding::ASCII), 0};
}

// Guesses the encoding of binary string data.
[[nodiscard]] inline encoding_info guess_encoding(const std::vector<BYTE>& data) noexcept
{
	return guess_encoding(&data[0], data.size());
}

// Parses the binary data into string.
// Guesses the encoding.
[[nodiscard]] inline std::wstring parse_str(const BYTE* data, size_t sz)
{
	if (!data || !sz) return {}; // nothing to parse

	encoding_info fileEnc = guess_encoding(data, sz);
	data += fileEnc.bomSize; // skip BOM, if any

	switch (fileEnc.encType) {
		case encoding::UNKNOWN:
		case encoding::ASCII:   return _wli::str_from_ansi(data, sz);
		case encoding::WIN1252: return _wli::str_from_code_page(data, sz, 1252);
		case encoding::UTF8:    return _wli::str_from_code_page(data, sz, CP_UTF8);
		case encoding::UTF16BE: return _wli::str_from_utf16(data, sz, true);
		case encoding::UTF16LE: return _wli::str_from_utf16(data, sz, false);
		case encoding::UTF32BE: throw std::invalid_argument("UTF-32 big endian: encoding not implemented.");
		case encoding::UTF32LE: throw std::invalid_argument("UTF-32 little endian: encoding not implemented.");
		case encoding::SCSU:    throw std::invalid_argument("Standard compression scheme for Unicode: encoding not implemented.");
		case encoding::BOCU1:   throw std::invalid_argument("Binary ordered compression for Unicode: encoding not implemented.");
		default:                throw std::invalid_argument("Unknown encoding.");
	}
}

// Parses binary data into string, guessing the encoding.
[[nodiscard]] inline std::wstring parse_str(const std::vector<BYTE>& data)
{
	return parse_str(&data[0], data.size());
}

// Parses binary data into uint16, big-endian.
[[nodiscard]] inline WORD parse_uint16_be(const BYTE* data) noexcept
{
	return _wli::parse_uint16_be(data);
}

// Parses binary data into uint16, little-endian.
[[nodiscard]] inline WORD parse_uint16_le(const BYTE* data) noexcept
{
	return _wli::parse_uint16_le(data);
}

// Parses binary data into uint32, big-endian.
[[nodiscard]] inline DWORD parse_uint32_be(const BYTE* data) noexcept
{
	return _wli::parse_uint32_be(data);
}

// Parses binary data into uint32, little-endian.
[[nodiscard]] inline DWORD parse_uint32_le(const BYTE* data) noexcept
{
	return _wli::parse_uint32_le(data);
}

// Writes uint16 into binary buffer, big-endian.
inline void put_uint16_be(WORD n, BYTE* dest) noexcept
{
	dest[0] = (n & 0xFF00) >> 8;
	dest[1] = n & 0xFF;
}

// Writes uint16 into binary buffer, little-endian.
inline void put_uint16_le(WORD n, BYTE* dest) noexcept
{
	dest[0] = n & 0xFF;
	dest[1] = (n & 0xFF00) >> 8;
}

// Writes uint32 into binary buffer, big-endian.
inline void put_uint32_be(DWORD n, BYTE* dest) noexcept
{
	dest[0] = static_cast<BYTE>((n & 0xFF000000) >> 24);
	dest[1] = static_cast<BYTE>((n & 0xFF0000) >> 16);
	dest[2] = static_cast<BYTE>((n & 0xFF00) >> 8);
	dest[3] = static_cast<BYTE>(n & 0xFF);
}

// Writes uint32 into binary buffer, little-endian.
inline void put_uint32_le(DWORD n, BYTE* dest) noexcept
{
	dest[0] = static_cast<BYTE>(n & 0xFF);
	dest[1] = static_cast<BYTE>((n & 0xFF00) >> 8);
	dest[2] = static_cast<BYTE>((n & 0xFF0000) >> 16);
	dest[3] = static_cast<BYTE>((n & 0xFF000000) >> 24);
}

// Converts a string into UTF-8 binary data with WideCharToMultiByte().
inline std::vector<BYTE> str_to_utf8(std::wstring_view s)
{
	std::vector<BYTE> ret;

	if (!s.empty()) {
		int neededLen = WideCharToMultiByte(CP_UTF8, 0,
			s.data(), static_cast<int>(s.length()),
			nullptr, 0, nullptr, 0);
		ret.resize(neededLen);

		WideCharToMultiByte(CP_UTF8, 0,
			s.data(), static_cast<int>(s.length()),
			reinterpret_cast<char*>(&ret[0]),
			neededLen, nullptr, nullptr);
	}
	return ret;
}

}//namespace wl::bin