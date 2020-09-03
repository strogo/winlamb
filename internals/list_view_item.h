/**
 * Part of WinLamb - Win32 API Lambda Library
 * https://github.com/rodrigocfd/winlamb
 * This library is released under the MIT License
 */

#pragma once
#include <Windows.h>

namespace wl {

// A single item of the list view.
class list_view_item final {
private:
	HWND _hList = nullptr;
	size_t _index = -1;

public:
	explicit list_view_item(HWND owner, size_t index) noexcept :
		_hList{owner}, _index{index} { }

	// Returns the zero-based index of this item.
	[[nodiscard]] size_t index() const noexcept { return this->_index; }

	[[nodiscard]] bool operator==(const list_view_item& other) const noexcept
	{
		return this->_hList == other._hList
			&& this->_index == other._index;
	}

	[[nodiscard]] bool operator!=(const list_view_item& other) const noexcept
	{
		return !this->operator==(other);
	}

	// Sends LVM_ENSUREVISIBLE.
	const list_view_item& ensure_visible() const
	{
		if (!ListView_EnsureVisible(this->_hList, this->_index, 1)) {
			throw std::runtime_error("LVM_ENSUREVISIBLE failed.");
		}
		return *this;
	}

	// Focus the item.
	const list_view_item& focus() const noexcept
	{
		ListView_SetItemState(this->_hList, this->_index, LVIS_FOCUSED, LVIS_FOCUSED);
		return *this;
	}

	// Returns the index of the image list icon of an item.
	[[nodiscard]] int icon_index() const
	{
		LVITEMW lvi{};
		lvi.iItem = static_cast<int>(this->_index);
		lvi.mask = LVIF_IMAGE;

		if (!ListView_GetItem(this->_hList, &lvi)) {
			throw std::runtime_error("LVM_GETITEM failed for icon index.");
		}
		return lvi.iImage;
	}

	// Returns the LPARAM associated to the item.
	[[nodiscard]] LPARAM param() const
	{
		LVITEMW lvi{};
		lvi.iItem = static_cast<int>(this->_index);
		lvi.mask = LVIF_PARAM;

		if (!ListView_GetItem(this->_hList, &lvi)) {
			throw std::runtime_error("LVM_GETITEM failed for param.");
		}
		return lvi.lParam;
	}

	// Sends LVM_GETITEMRECT for an item.
	// Returns coordinates relative to list view.
	[[nodiscard]] RECT rect(int lvirPortion = LVIR_BOUNDS) const
	{
		RECT rcItem{};
		rcItem.left = lvirPortion;

		if (!ListView_GetItemRect(this->_hList, this->_index, &rcItem, lvirPortion)) {
			throw std::runtime_error("LVM_GETITEMRECT failed.");
		}
		return rcItem;
	}

	// Deletes the item with LVM_DELETEITEM.
	const list_view_item& remove() const
	{
		if (!ListView_DeleteItem(this->_hList, this->_index)) {
			throw std::runtime_error("LVM_DELETEITEM failed.");
		}
		return *this;
	}

	// Selects or deselects the item.
	const list_view_item& select(bool isSelected) const noexcept
	{
		ListView_SetItemState(this->_hList, this->_index,
			isSelected ? LVIS_SELECTED : 0, LVIS_SELECTED);
		return *this;
	}

	// Sets the index of the image list icon for an item.
	const list_view_item& set_icon(int iconIndex) const
	{
		LVITEMW lvi{};
		lvi.iItem = static_cast<int>(this->_index);
		lvi.mask = LVIF_IMAGE;
		lvi.iImage = iconIndex;

		if (!ListView_SetItem(this->_hList, &lvi)) {
			throw std::runtime_error("LVM_SETITEM failed for icon index.");
		}
		return *this;
	}

	// Sets the LPARAM associated to the item.
	const list_view_item& set_param(LPARAM lp) const
	{
		LVITEMW lvi{};
		lvi.iItem = static_cast<int>(this->_index);
		lvi.mask = LVIF_PARAM;
		lvi.lParam = lp;

		if (!ListView_SetItem(this->_hList, &lvi)) {
			throw std::runtime_error("LVM_SETITEM failed for param.");
		}
		return *this;
	}

	// Sets the text of an item at the given column with LVM_SETITEMTEXT.
	const list_view_item& set_subitem_text(
		size_t columnIndex, std::wstring_view text) const
	{
		LVITEMW lvi{};
		lvi.iSubItem = static_cast<int>(columnIndex);
		lvi.pszText = const_cast<wchar_t*>(text.data());

		if (!SendMessageW(this->_hList, LVM_SETITEMTEXT,
			this->_index, reinterpret_cast<LPARAM>(&lvi)) )
		{
			throw std::runtime_error(
				wl::str::unicode_to_ansi(
					wl::str::format(L"LVM_SETITEMTEXT failed to set text \"%s\".", text) ));
		}
		return *this;
	}

	// Sets the text of an item, with LVM_SETITEMTEXT.
	const list_view_item& set_text(std::wstring_view text) const
	{
		return this->set_subitem_text(0, text);
	}

	// Retrieves the text of an item with LVM_GETITEMTEXT.
	[[nodiscard]] std::wstring text() const
	{
		return this->subitem_text(0);
	}

	// Retrieves the text of an item at the given column with LVM_GETITEMTEXT.
	[[nodiscard]] std::wstring subitem_text(size_t columnIndex) const
	{
		// http://forums.codeguru.com/showthread.php?351972-Getting-listView-item-text-length
		LVITEMW lvi{};
		lvi.iSubItem = static_cast<int>(columnIndex);

		// Notice that, since strings' size always increase, if the buffer
		// was previously allocated with a value bigger than our 1st step,
		// this will speed up the size checks.

		std::wstring buf(64, L'\0'); // speed-up 1st allocation
		int baseBufLen = 0;
		int charsWrittenWithoutNull = 0;

		do {
			baseBufLen += 64; // buffer increasing step, arbitrary!
			buf.resize(baseBufLen);
			lvi.cchTextMax = baseBufLen;
			lvi.pszText = &buf[0];
			charsWrittenWithoutNull = static_cast<int>(
				SendMessageW(this->_hList, LVM_GETITEMTEXT,
					this->_index, reinterpret_cast<LPARAM>(&lvi)) );
		} while (charsWrittenWithoutNull == baseBufLen - 1); // to break, must have at least 1 char gap

		buf.resize( lstrlenW(buf.c_str()) ); // trim nulls
		return buf;
	}

	// Sends LVM_UPDATE.
	const list_view_item& update() const
	{
		if (!ListView_Update(this->_hList, this->_index)) {
			throw std::runtime_error("LVM_UPDATE failed.");
		}
		return *this;
	}

	// Tells if the item is visible by sending LVM_ISITEMVISIBLE.
	[[nodiscard]] bool visible() const noexcept
	{
		return ListView_IsItemVisible(this->_hList, this->_index) != 0;
	}
};

}//namespace wl