/**
 * Part of WinLamb - Win32 API Lambda Library
 * https://github.com/rodrigocfd/winlamb
 * This library is released under the MIT License
 */

#pragma once
#include <optional>
#include <stdexcept>
#include <vector>
#include <Windows.h>
#include <CommCtrl.h>
#include "internals/base_native_control.h"
#include "internals/control_visuals.h"
#include "internals/interfaces.h"
#include "internals/list_view_column.h"
#include "internals/list_view_item.h"
#include "internals/param.h"
#include "image_list.h"
#include "menu.h"
#include "str.h"

namespace wl {

// Native list view control.
// https://docs.microsoft.com/en-us/windows/win32/controls/list-view-controls-overview
class list_view final : public i_control {
private:
	_wli::base_native_control _base;
	menu _contextMenu;

public:
	list_view()
	{
		this->_add_default_subclass_handlers();
	}

	list_view(list_view&&) = default;
	list_view& operator=(list_view&&) = default; // movable only

	// Returns the HWND.
	[[nodiscard]] HWND hwnd() const noexcept override { return this->_base.hwnd(); }

	// Retrieves the control ID.
	[[nodiscard]] int id() const noexcept override { return this->_base.id(); }

	// Calls EnableWindow().
	const list_view& enable(bool isEnabled) const noexcept { EnableWindow(this->hwnd(), isEnabled); return *this; }

	// In a dialog window, assigns to a control.
	list_view& assign(const i_window* parent, int ctrlId) { this->_base.assign(parent, ctrlId); return *this; }

	// Installs a window subclass and adds a handle to a message.
	// func: [](wl::param::any p) -> LRESULT { }
	template<typename F>
	void on_subclass_message(UINT msg, F&& func) { this->_base.on_subclass_message(msg, std::move(func)); }

	// Installs a window subclass and adds a handle to a message.
	// func: [](wl::param::any p) -> LRESULT { }
	template<typename F>
	void on_subclass_message(std::initializer_list<UINT> msgs, F&& func) { this->_base.on_subclass_message(msgs, std::move(func)); }

	// Returns the column at the given index.
	// Does not perform bound checking.
	[[nodiscard]] list_view_column column(size_t columnIndex) const noexcept { return list_view_column{this->hwnd(), columnIndex}; }

	// Returns the item at the given index.
	// Does not perform bound checking.
	[[nodiscard]] list_view_item item(size_t itemIndex) const noexcept { return list_view_item{this->hwnd(), itemIndex}; }

	// Calls CreateWindowEx().
	// This method will always add LVS_SHAREIMAGELISTS flag, for safety.
	// Should be called during parent's WM_CREATE processing.
	// Position and size will be adjusted to match current system DPI.
	list_view& create(const i_window* parent, int id, POINT pos, SIZE size,
		DWORD lvStyles = 0, DWORD lvExStyles = 0)
	{
		HWND h = this->_base.create_window(parent, id, WC_LISTVIEWW, {}, pos, size,
			WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_GROUP | LVS_SHAREIMAGELISTS | lvStyles, // never own image lists
			WS_EX_CLIENTEDGE);

		pos = _wli::multiply_dpi(pos);
		size = _wli::multiply_dpi(size);

		if (lvExStyles) {
			this->set_extended_lv_style(true, lvExStyles);
		}
		return *this;
	}

	// Adds a new column.
	const list_view& add_column(std::wstring_view text, UINT width) const
	{
		LVCOLUMNW lvc{};
		lvc.mask = LVCF_TEXT | LVCF_WIDTH;
		lvc.pszText = const_cast<wchar_t*>(text.data());
		lvc.cx = width;

		if (SendMessageW(this->hwnd(), LVM_INSERTCOLUMN,
			0xFFFF, reinterpret_cast<LPARAM>(&lvc)) == -1)
		{
			throw std::runtime_error(
				wl::str::unicode_to_ansi(
					wl::str::format(L"LVM_INSERTCOLUMN failed \"%s\".", text) ));
		}
		return *this;
	}

	// Adds a new item.
	// Returns the newly added item.
	list_view_item add_item(std::wstring_view text) const
	{
		LVITEMW lvi{};
		lvi.mask = LVIF_TEXT;
		lvi.pszText = const_cast<wchar_t*>(text.data());
		lvi.iItem = 0x0FFF'FFFF; // insert as the last one

		size_t newIdx = ListView_InsertItem(this->hwnd(), &lvi);
		if (newIdx == -1) {
			throw std::runtime_error(
				wl::str::unicode_to_ansi(
					wl::str::format(L"LVM_INSERTITEM failed \"%s\".", text) ));
		}
		return list_view_item{this->hwnd(), newIdx};
	}

	// Adds a new item.
	// You must attach an image list to see the icons.
	// Returns the newly added item.
	list_view_item add_item_with_icon(std::wstring_view text, int iIcon) const
	{
		LVITEMW lvi{};
		lvi.mask = LVIF_TEXT | LVIF_IMAGE;
		lvi.pszText = const_cast<wchar_t*>(text.data());
		lvi.iItem = 0x0FFF'FFFF; // insert as the last one
		lvi.iImage = iIcon;

		size_t newIdx = ListView_InsertItem(this->hwnd(), &lvi);
		if (newIdx == -1) {
			throw std::runtime_error(
				wl::str::unicode_to_ansi(
					wl::str::format(L"LVM_INSERTITEM failed \"%s\".", text) ));
		}
		return list_view_item{this->hwnd(), newIdx};
	}

	// Retrieves all items in the list view.
	std::vector<list_view_item> all_items() const
	{
		std::vector<list_view_item> items;
		items.reserve(this->item_count());

		int idx = -1;
		for (;;) {
			idx = ListView_GetNextItem(this->hwnd(), idx, LVNI_ALL);
			if (idx == -1) break;

			items.emplace_back(this->item(idx));
		}

		return items;
	}

	// Retrieves the texts of all items, at the given column.
	std::vector<std::wstring> all_items_text(size_t columnIndex) const
	{
		std::vector<std::wstring> texts;
		texts.reserve(this->item_count());

		int idx = -1;
		for (;;) {
			idx = ListView_GetNextItem(this->hwnd(), idx, LVNI_ALL);
			if (idx == -1) break;

			list_view_item theItem = this->item(idx);
			texts.emplace_back(theItem.subitem_text(columnIndex));
		}

		return texts;
	}

	// Retrieves the number of columns.
	[[nodiscard]] size_t column_count() const
	{
		return _wli::list_view_column_count(this->hwnd());
	}

	// Retrieves the LVS_EX style.
	[[nodiscard]] DWORD extended_lv_style() const noexcept
	{
		return static_cast<DWORD>(
			SendMessageW(this->hwnd(), LVM_GETEXTENDEDLISTVIEWSTYLE, 0, 0) );
	}

	// Retrieves the item with the given text, case-insensitive, if any.
	[[nodiscard]] std::optional<list_view_item> find(std::wstring_view text) const noexcept
	{
		LVFINDINFOW lfi{};
		lfi.flags = LVFI_STRING;
		lfi.psz = text.data();

		int idx = ListView_FindItem(this->hwnd(), -1, &lfi);
		if (idx == -1) {
			return {};
		}
		return {this->item(idx)};
	}

	// Retrieves the currently focused item, if any.
	[[nodiscard]] std::optional<list_view_item> focused_item() const noexcept
	{
		int idx = ListView_GetNextItem(this->hwnd(), -1, LVNI_FOCUSED);
		if (idx == -1) {
			return {};
		}
		return {this->item(idx)};
	}

	// Sends LVM_HITTEST to determine the item at specified position, if any.
	// Position coordinates must be relative to list view.
	[[nodiscard]] LVHITTESTINFO hit_test(POINT pos) const noexcept
	{
		LVHITTESTINFO lvht{};
		lvht.pt = pos;

		SendMessageW(this->hwnd(), LVM_HITTEST,
			-1, reinterpret_cast<LPARAM>(&lvht));
		return lvht;
	}

	// Retrieves the number of items.
	[[nodiscard]] size_t item_count() const noexcept
	{
		return ListView_GetItemCount(this->hwnd());
	}

	// Deletes all items with LVM_DELETEALLITEMS.
	const list_view& remove_all_items() const
	{
		if (!ListView_DeleteAllItems(this->hwnd())) {
			throw std::runtime_error("LVM_DELETEALLITEMS failed.");
		}
		return *this;
	}

	// Deletes the given items.
	const list_view& remove_items(const std::vector<list_view_item>& items) const
	{
		for (size_t i = items.size(); i--; ) {
			items[i].remove();
		}
		return *this;
	}

	// Deletes the given items.
	const list_view& remove_items(std::initializer_list<size_t> indexes) const
	{
		for (size_t idx : indexes) {
			if (!ListView_DeleteItem(this->hwnd(), idx)) {
				throw std::runtime_error("LVM_DELETEITEM failed.");
			}
		}
		return *this;
	}

	// Deletes all items currently selected.
	const list_view& remove_selected_items() const noexcept
	{
		int i = -1;
		while ((i = ListView_GetNextItem(this->hwnd(), -1, LVNI_SELECTED)) != -1) {
			ListView_DeleteItem(this->hwnd(), i);
		}
		return *this;
	}

	// Selects or deselects all items.
	const list_view& select_all_items(bool isSelected) const noexcept
	{
		ListView_SetItemState(this->hwnd(), -1,
			isSelected ? LVIS_SELECTED : 0, LVIS_SELECTED);
		return *this;
	}

	// Selects or deselects the given items.
	const list_view& select_items(
		const std::vector<list_view_item>& items, bool isSelected) const noexcept
	{
		for (const list_view_item& i : items) {
			i.select(isSelected);
		}
		return *this;
	}

	// Selects or deselects the given items.
	const list_view& select_items(
		std::initializer_list<size_t> indexes, bool isSelected) const noexcept
	{
		for (size_t idx : indexes) {
			ListView_SetItemState(this->hwnd(), idx,
				isSelected ? LVIS_SELECTED : 0, LVIS_SELECTED);
		}
		return *this;
	}

	// Retrieves the number of selected items.
	[[nodiscard]] size_t selected_item_count() const noexcept
	{
		return ListView_GetSelectedCount(this->hwnd());
	}

	// Retrieves the selected items, if any.
	[[nodiscard]] std::vector<list_view_item> selected_items() const
	{
		std::vector<list_view_item> items;
		items.reserve(this->selected_item_count());

		int idx = -1;
		for (;;) {
			idx = ListView_GetNextItem(this->hwnd(), idx, LVNI_SELECTED);
			if (idx == -1) break;
			items.emplace_back(this->hwnd(), idx);
		}
		return items;
	}

	// Retrieves the texts of the selected items, at the given column.
	[[nodiscard]] std::vector<std::wstring> selected_items_text(size_t columnIndex) const
	{
		std::vector<std::wstring> texts;
		texts.reserve(this->selected_item_count());

		int idx = -1;
		for (;;) {
			idx = ListView_GetNextItem(this->hwnd(), idx, LVNI_SELECTED);
			if (idx == -1) break;

			list_view_item theItem = this->item(idx);
			texts.emplace_back(theItem.subitem_text(columnIndex));
		}

		return texts;
	}

	// Sets a popup menu to be used as the context menu.
	// The list view doesn't own the menu, and it must remain valid to be used.
	list_view& set_context_menu(const menu& popupMenu) noexcept
	{
		this->_contextMenu = popupMenu;
		return *this;
	}

	// Sets the LVS_EX style signaled by the LVS_EX mask.
	const list_view& set_extended_lv_style(bool isSet, DWORD lvExStyles) const noexcept
	{
		SendMessageW(this->hwnd(), LVM_SETEXTENDEDLISTVIEWSTYLE,
			lvExStyles, isSet ? lvExStyles : 0);
		return *this;
	}

	// Sets the associated image list with ListView_SetImageList().
	// The list view doesn't own the image list, and it must remain valid to be used.
	// The lvsilType defaults to LVSIL_SMALL.
	list_view& set_image_list(
		const image_list& imageList, DWORD lvsilType = LVSIL_SMALL) noexcept
	{
		// This method is non-const because it's usually called during object creation,
		// which chains many non-const methods.
		ListView_SetImageList(this->hwnd(), imageList.himagelist(), lvsilType);
		return *this;
	}

	// Sends a WM_SETREDRAW.
	// Value false prevents changes the window from being redrawn.
	const list_view& set_redraw(bool doRedraw) const noexcept
	{
		SendMessageW(this->hwnd(), WM_SETREDRAW,
			static_cast<WPARAM>(static_cast<BOOL>(doRedraw)), 0);
		return *this;
	}

private:
	void _add_default_subclass_handlers()
	{
		this->on_subclass_message(WM_GETDLGCODE, [this](param::wm::getdlgcode p) noexcept -> LRESULT
		{
			if (!p.is_query() && p.vkey_code() == 'A' && p.has_ctrl()) { // Ctrl+A to select all items
				this->select_all_items(true);
				return DLGC_WANTCHARS;
			} else if (!p.is_query() && p.vkey_code() == VK_RETURN) { // send Enter key to parent
#pragma warning (disable: 26454) // https://stackoverflow.com/a/51142504/6923555
				NMLVKEYDOWN nmlvkd = {
					{
						this->hwnd(),
						static_cast<WORD>(this->id()),
						LVN_KEYDOWN // this triggers warning 26454: arithmetic overflow
					},
					VK_RETURN, 0
				};
#pragma warning (default: 26454)
				SendMessageW(GetAncestor(this->hwnd(), GA_PARENT),
					WM_NOTIFY, reinterpret_cast<WPARAM>(this->hwnd()),
					reinterpret_cast<LPARAM>(&nmlvkd));
				return DLGC_WANTALLKEYS;
			} else if (!p.is_query() && p.vkey_code() == VK_APPS) { // context menu keyboard key
				this->_show_context_menu(false, p.has_ctrl(), p.has_shift());
			}
			return DefSubclassProc(this->hwnd(), WM_GETDLGCODE, p.wparam, p.lparam);
		});

		this->on_subclass_message(WM_RBUTTONDOWN, [this](param::wm::rbuttondown p) noexcept -> LRESULT
		{
			this->_show_context_menu(true, p.has_ctrl(), p.has_shift());
			return 0;
		});
	}

	int _show_context_menu(bool followCursor, bool hasCtrl, bool hasShift) noexcept
	{
		if (!this->_contextMenu.hmenu()) return -1; // no context menu assigned

		POINT coords{};
		int itemBelowCursor = -1;
		if (followCursor) { // usually fired with a right-click
			LVHITTESTINFO lvhti{};
			GetCursorPos(&lvhti.pt); // relative to screen
			ScreenToClient(this->hwnd(), &lvhti.pt); // now relative to list view
			ListView_HitTest(this->hwnd(), &lvhti); // item below cursor, if any
			coords = lvhti.pt;
			itemBelowCursor = lvhti.iItem; // -1 if none
			if (itemBelowCursor != -1) { // an item was right-clicked
				if (!hasCtrl && !hasShift) {
					if ((ListView_GetItemState(this->hwnd(), itemBelowCursor, LVIS_SELECTED) & LVIS_SELECTED) == 0) {
						// If right-clicked item isn't currently selected, unselect all and select just it.
						ListView_SetItemState(this->hwnd(), -1, 0, LVIS_SELECTED);
						ListView_SetItemState(this->hwnd(), itemBelowCursor, LVIS_SELECTED, LVIS_SELECTED);
					}
					ListView_SetItemState(this->hwnd(), itemBelowCursor, LVIS_FOCUSED, LVIS_FOCUSED); // focus clicked
				}
			} else if (!hasCtrl && !hasShift) {
				ListView_SetItemState(this->hwnd(), -1, 0, LVIS_SELECTED); // unselect all
			}
			SetFocus(this->hwnd()); // because a right-click won't set the focus by default
		} else { // usually fired with the context menu keyboard key
			int itemFocused = ListView_GetNextItem(this->hwnd(), -1, LVNI_FOCUSED);
			if (itemFocused != -1 && ListView_IsItemVisible(this->hwnd(), itemFocused)) { // item focused and visible
				RECT rcItem{};
				ListView_GetItemRect(this->hwnd(), itemFocused, &rcItem, LVIR_BOUNDS); // relative to list view
				coords = {rcItem.left + 16, rcItem.top + (rcItem.bottom - rcItem.top) / 2};
			} else { // no focused and visible item
				coords = {6, 10};
			}
		}

		// The popup menu is created with hDlg as parent, so the menu messages go to it.
		// The lvhti coordinates are relative to list view, and will be mapped into screen-relative.
		this->_contextMenu.show_at_point(GetParent(this->hwnd()), coords, this->hwnd());
		return itemBelowCursor; // -1 if none
	}
};

}//namespace wl