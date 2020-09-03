/**
 * Part of WinLamb - Win32 API Lambda Library
 * https://github.com/rodrigocfd/winlamb
 * This library is released under the MIT License
 */

#pragma once
#include <stdexcept>
#include <string_view>
#include <Windows.h>

namespace _wli {

// Declared outside all classes because it's used in a couple different places.
[[nodiscard]] inline size_t list_view_column_count(HWND hListView)
{
	HWND hHeader = reinterpret_cast<HWND>(
		SendMessageW(hListView, LVM_GETHEADER, 0, 0) );
	if (!hHeader) {
		throw std::runtime_error("LVM_GETHEADER failed when retrieving column count.");
	}

	size_t count = SendMessageW(hHeader, HDM_GETITEMCOUNT, 0, 0);
	if (count == -1) {
		throw std::runtime_error("HDM_GETITEMCOUNT failed.");
	}
	return count;
}

}//namespace _wli

namespace wl {

// A single column of a list view.
class list_view_column final {
private:
	HWND _hList = nullptr;
	size_t _index = -1;

public:
	explicit list_view_column(HWND owner, size_t index) noexcept :
		_hList{owner}, _index{index} { }

	// Returns the zero-based index of this column.
	[[nodiscard]] size_t index() const noexcept { return this->_index; }

	[[nodiscard]] bool operator==(const list_view_column& other) const noexcept
	{
		return this->_hList == other._hList
			&& this->_index == other._index;
	}

	[[nodiscard]] bool operator!=(const list_view_column& other) const noexcept
	{
		return !this->operator==(other);
	}

	// Sets the text with LVM_SETCOLUMN.
	const list_view_column& set_text(std::wstring_view text) const
	{
		LVCOLUMNW lvc{};
		lvc.iSubItem = static_cast<int>(this->_index);
		lvc.mask = LVCF_TEXT;
		lvc.pszText = const_cast<wchar_t*>(text.data());

		if (!SendMessageW(this->_hList, LVM_SETCOLUMN,
			this->_index, reinterpret_cast<LPARAM>(&lvc)))
		{
			throw std::runtime_error(
				wl::str::unicode_to_ansi(
					wl::str::format(L"LVM_SETCOLUMN failed to set text \"%s\".", text) ));
		}
		return *this;
	}

	// Sets the width in pixels with LVM_SETCOLUMNWIDTH.
	const list_view_column& set_width(UINT cx) const
	{
		if (!SendMessageW(this->_hList, LVM_SETCOLUMNWIDTH, this->_index, cx)) {
			throw std::runtime_error("LVM_SETCOLUMNWIDTH failed.");
		}
		return *this;
	}

	// Expands or shrinks the column to fill the remaning space.
	const list_view_column& set_width_to_fill() const
	{
		size_t numCols = _wli::list_view_column_count(this->_hList);
		size_t cxUsed = 0;

		for (size_t i = 0; i < numCols; ++i) {
			if (i != this->_index) { // retrieve cx of each column, but chosen one
				size_t cx = static_cast<size_t>(
					SendMessageW(this->_hList, LVM_GETCOLUMNWIDTH, i, 0) );
				if (!cx) {
					throw std::runtime_error("LVM_GETCOLUMNWIDTH failed in set_width_to_fill.");
				}
				cxUsed += cx;
			}
		}

		RECT rc{};
		GetClientRect(this->_hList, &rc); // list view client area
		this->set_width(rc.right
			//- GetSystemMetrics(SM_CXVSCROLL)
			- static_cast<LONG>(cxUsed)); // fill available space
		return *this;
	}

	// Retrieves the text with LVM_GETCOLUMN.
	[[nodiscard]] std::wstring text() const
	{
		std::wstring buf(128, L'\0'); // arbitrary

		LVCOLUMNW lvc{};
		lvc.iSubItem = static_cast<int>(this->_index);
		lvc.mask = LVCF_TEXT;
		lvc.pszText = &buf[0];
		lvc.cchTextMax = static_cast<int>(buf.length());

		if (!SendMessageW(this->_hList, LVM_GETCOLUMN,
			this->_index, reinterpret_cast<LPARAM>(&lvc)) )
		{
			throw std::runtime_error("LVM_GETCOLUMN failed.");
		}
		return buf;
	}

	// Retrieves the width in pixels with LVM_GETCOLUMNWIDTH.
	[[nodiscard]] size_t width() const
	{
		size_t cx = static_cast<size_t>(
			SendMessageW(this->_hList, LVM_GETCOLUMNWIDTH, this->_index, 0) );

		if (!cx) {
			throw std::runtime_error("LVM_GETCOLUMNWIDTH failed.");
		}
		return cx;
	}
};

}//namespace wl