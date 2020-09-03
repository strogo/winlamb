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

namespace wl::com {

// Manages a COM VARIANT object.
class variant final {
private:
	VARIANT _vari{};

public:
	~variant() { this->clear(); }
	variant() = default;
	variant(variant&& other) noexcept { this->operator=(std::move(other)); }

	[[nodiscard]] operator const VARIANT&() const noexcept  { return this->_vari; }
	[[nodiscard]] const VARIANT* operator&() const noexcept { return &this->_vari; }
	[[nodiscard]] VARIANT* operator&() noexcept             { return &this->_vari; }

	variant& operator=(variant&& other) noexcept
	{
		this->clear();
		std::swap(this->_vari, other._vari);
		return *this;
	}

	// Retrieves a stored boolean value.
	// Throws exception if the stored type is different.
	[[nodiscard]] bool boolean() const
	{
		if (this->_vari.vt != VT_BOOL) {
			throw std::invalid_argument("Variant doesn't hold a boolean.");
		}
		return this->_vari.boolVal != VARIANT_FALSE;
	}

	// Retrieves a stored BYTE value.
	// Throws exception if the stored type is different.
	[[nodiscard]] BYTE byte() const
	{
		if (this->_vari.vt != VT_UI1) {
			throw std::invalid_argument("Variant doesn't hold a BYTE.");
		}
		return this->_vari.bVal;
	}

	// Clears the current stored value.
	variant& clear() noexcept
	{
		if (this->_vari.vt != VT_EMPTY) {
			VariantClear(&this->_vari); // will set VT_EMPTY
		}
		return *this;
	}

	// Retrieves a stored string value.
	// Throws exception if the stored type is different.
	[[nodiscard]] IDispatch* idispatch() const
	{
		if (this->_vari.vt != VT_DISPATCH) {
			throw std::invalid_argument("Variant doesn't hold an IDispatch.");
		}
		return this->_vari.pdispVal;
	}

	// Retrieves a stored 16-bit int value.
	// Throws exception if the stored type is different.
	[[nodiscard]] short int16() const
	{
		if (this->_vari.vt != VT_I2) {
			throw std::invalid_argument("Variant doesn't hold a 16-bit int.");
		}
		return this->_vari.iVal;
	}

	// Retrieves a stored 32-bit int value.
	// Throws exception if the stored type is different.
	[[nodiscard]] int int32() const
	{
		if (this->_vari.vt != VT_I4) {
			throw std::invalid_argument("Variant doesn't hold a 32-bit int.");
		}
		return this->_vari.intVal;
	}

	// Stores a boolean value.
	variant& set_boolean(bool b) noexcept { return this->_set_num(VT_BOOL, this->_vari.boolVal, b ? VARIANT_TRUE : VARIANT_FALSE); }

	// Stores a BYTE value.
	variant& set_byte(BYTE n) noexcept { return this->_set_num(VT_UI1, this->_vari.bVal, n); }

	// Stores an IDispatch derived value.
	template<typename idispatch_derivedT>
	variant& set_idispatch(ptr<idispatch_derivedT>& objToQueryFrom)
	{
		this->clear();
		this->_vari.vt = VT_DISPATCH;

		HRESULT hr = objToQueryFrom->QueryInterface(
			IID_IDispatch, reinterpret_cast<void**>(&this->_vari.pdispVal));
		if (FAILED(hr)) {
			throw std::system_error(hr, std::system_category(),
				"QueryInterface failed in com_variant::set_idispatch.");
		}

		return *this;
	}

	// Stores a 16-bit int value.
	variant& set_int16(short n) noexcept { return this->_set_num(VT_I2, this->_vari.iVal, n); }

	// Stores a 32-bit int value.
	variant& set_int32(int n) noexcept { return this->_set_num(VT_I4, this->_vari.intVal, n); }

	// Stores an unsigned 16-bit int value.
	variant& set_uint16(unsigned short n) noexcept { return this->_set_num(VT_UI2, this->_vari.uiVal, n); }

	// Stores an unsigned 32-bit int value.
	variant& set_uint32(unsigned int n) noexcept { return this->_set_num(VT_UI4, this->_vari.uintVal, n); }

	// Stores a string value.
	variant& set_str(std::wstring_view s) noexcept
	{
		this->clear();
		this->_vari.vt = VT_BSTR;
		this->_vari.bstrVal = SysAllocString(s.data());
		return *this;
	}

	// Retrieves a stored string value.
	// Throws exception if the stored type is different.
	[[nodiscard]] const wchar_t* str() const
	{
		if (this->_vari.vt != VT_BSTR) {
			throw std::invalid_argument("Variant doesn't hold a string.");
		}
		return this->_vari.bstrVal;
	}

	// Retrieves a stored unsigned 16-bit int value.
	// Throws exception if the stored type is different.
	[[nodiscard]] unsigned short uint16() const
	{
		if (this->_vari.vt != VT_UI2) {
			throw std::invalid_argument("Variant doesn't hold an unsigned 16-bit int.");
		}
		return this->_vari.uiVal;
	}

	// Retrieves a stored unsigned 32-bit int value.
	// Throws exception if the stored type is different.
	[[nodiscard]] unsigned int uint32() const
	{
		if (this->_vari.vt != VT_UI4) {
			throw std::invalid_argument("Variant doesn't hold an unsigned 32-bit int.");
		}
		return this->_vari.uintVal;
	}

private:
	template<typename T>
	variant& _set_num(VARTYPE vt, T& dest, T val) noexcept
	{
		this->clear();
		this->_vari.vt = vt;
		dest = val;
		return *this;
	}
};

}//namespace wl::com