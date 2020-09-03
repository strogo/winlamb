/**
 * Part of WinLamb - Win32 API Lambda Library
 * https://github.com/rodrigocfd/winlamb
 * This library is released under the MIT License
 */

#pragma once
#include <stdexcept>
#include <string_view>
#include <Windows.h>
#include <OleAuto.h>
#include "../str.h"

namespace wl::com {

// Manages a COM BSTR string.
class bstr final {
private:
	BSTR _bstr = nullptr;

public:
	~bstr() { this->free(); }
	bstr() = default;
	bstr(bstr&& other) noexcept { this->operator=(std::move(other)); }
	bstr(std::wstring_view s) { this->operator=(s); }

	[[nodiscard]] operator const BSTR&() const noexcept  { return this->_bstr; }
	[[nodiscard]] const BSTR* operator&() const noexcept { return &this->_bstr; }
	[[nodiscard]] BSTR* operator&() noexcept             { return &this->_bstr; }

	// Converts the BSTR into const wchar_t*.
	[[nodiscard]] const wchar_t* c_str() const noexcept { return static_cast<const wchar_t*>(this->_bstr); }

	bstr& operator=(bstr&& other) noexcept
	{
		this->free();
		std::swap(this->_bstr, other._bstr);
		return *this;
	}

	bstr& operator=(std::wstring_view s)
	{
		this->free();
		this->_bstr = SysAllocString(s.data());

		if (!this->_bstr) {
			throw std::runtime_error(
				str::unicode_to_ansi(
					str::format(L"SysAllocString failed \"%s\".", s) ));
		}
		return *this;
	}

	// Calls SysFreeString().
	void free() noexcept
	{
		if (this->_bstr) {
			SysFreeString(this->_bstr);
			this->_bstr = nullptr;
		}
	}
};

}//namespace wl::com