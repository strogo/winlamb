/**
 * Part of WinLamb - Win32 API Lambda Library
 * https://github.com/rodrigocfd/winlamb
 * This library is released under the MIT License
 */

#pragma once
#include <stdexcept>
#include <string_view>
#include <system_error>
#include <Windows.h>
#include "str.h"

namespace wl {

// Simply holds a menu handle (HMENU).
// https://docs.microsoft.com/en-us/windows/win32/menurc/about-menus
class menu {
protected:
	HMENU _hMenu = nullptr;

public:
	virtual ~menu() { }
	menu() = default;
	explicit menu(HMENU hMenu) noexcept : _hMenu{hMenu} { }
	menu(const menu&) = default;
	menu& operator=(const menu&) = default;

	bool operator==(const menu& other) const noexcept { return this->_hMenu == other._hMenu; }
	bool operator!=(const menu& other) const noexcept { return !this->operator==(other); }

	// Returns the HMENU.
	[[nodiscard]] HMENU hmenu() const noexcept { return this->_hMenu; }

	// Calls AppendMenu().
	const menu& append_item(int cmdId, std::wstring_view text) const
	{
		if (!AppendMenuW(this->_hMenu, MF_STRING, cmdId, text.data())) {
			throw std::system_error(GetLastError(), std::system_category(),
				"AppendMenu failed.");
		}
		return *this;
	}

	// Calls AppendMenu().
	const menu& append_separator() const
	{
		if (!AppendMenuW(this->_hMenu, MF_SEPARATOR, 0, nullptr)) {
			throw std::system_error(GetLastError(), std::system_category(),
				"AppendMenu failed.");
		}
		return *this;
	}

	// Calls CreateMenuPopup() and AppendMenu().
	// Returns the newly appended menu.
	menu append_submenu(std::wstring_view text) const
	{
		HMENU pop = _create_submenu();

		if (!AppendMenuW(this->_hMenu, MF_STRING | MF_POPUP,
			reinterpret_cast<UINT_PTR>(pop), text.data()))
		{
			throw std::system_error(GetLastError(), std::system_category(),
				"AppendMenu failed.");
		}

		return menu{pop};
	}

	// Retrieves the command ID of the menu item at the given position.
	// Throws exception if the item is out of range or a submenu.
	[[nodiscard]] int cmd_by_pos(size_t pos) const
	{
		int cmdId = GetMenuItemID(this->_hMenu, static_cast<int>(pos));
		if (cmdId == -1) {
			throw std::invalid_argument(
				str::unicode_to_ansi(
					str::format(L"Failed retrieve command ID of position %d.", pos) ));
		}
		return cmdId;
	}

	// Calls GetMenuItemCount() and then DeleteMenu() on each item.
	const menu& delete_all_items() const
	{
		for (size_t i = this->item_count(); i-- > 0; ) {
			this->delete_by_pos(i);
		}
		return *this;
	}

	// Calls DeleteMenu().
	const menu& delete_by_cmd(int cmdId) const { return this->_delete(cmdId, false); }
	// Calls DeleteMenu().
	const menu& delete_by_pos(size_t pos) const { return this->_delete(static_cast<UINT>(pos), true); }

	// Calls EnableMenuItem().
	const menu& enable_by_cmd(int cmdId, bool isEnabled) const { return this->_enable(cmdId, isEnabled, false); }
	// Calls EnableMenuItem().
	const menu& enable_by_pos(size_t pos, bool isEnabled) const { return this->_enable(static_cast<UINT>(pos), isEnabled, true); }

	// Calls EnableMenuItem().
	const menu& enable_by_cmd(std::initializer_list<int> cmdIds, bool isEnabled) const
	{
		for (int cmdId : cmdIds) {
			this->enable_by_cmd(cmdId, isEnabled);
		}
		return *this;
	}

	// Calls EnableMenuItem().
	const menu& enable_by_pos(std::initializer_list<size_t> poss, bool isEnabled) const
	{
		for (size_t pos : poss) {
			this->enable_by_pos(pos, isEnabled);
		}
		return *this;
	}

	// Calls InsertMenuItem().
	const menu& insert_item_before_cmd(
		int cmdIdBefore, int newCmdId, std::wstring_view text) const
	{
		return this->_insert_item_before(cmdIdBefore, newCmdId, text, false);
	}

	// Calls InsertMenuItem().
	const menu& insert_item_before_pos(
		size_t posBefore, int newCmdId, std::wstring_view text) const
	{
		return this->_insert_item_before(
			static_cast<UINT>(posBefore), newCmdId, text, true);
	}

	// Calls CreateMenuPopup() and InsertMenu().
	// Returns the newly inserted menu.
	menu insert_submenu_before_cmd(int cmdIdBefore, std::wstring_view text) const
	{
		return this->_insert_submenu_before(cmdIdBefore, text, false);
	}

	// Calls CreateMenuPopup() and InsertMenu().
	// Returns the newly inserted menu.
	menu insert_submenu_before_pos(size_t posBefore, std::wstring_view text) const
	{
		return this->_insert_submenu_before(
			static_cast<UINT>(posBefore), text, true);
	}

	// Calls GetMenuItemCount().
	size_t item_count() const
	{
		int count = GetMenuItemCount(this->_hMenu);
		if (count == -1) {
			throw std::system_error(GetLastError(), std::system_category(),
				"GetMenuItemCount failed.");
		}
		return count;
	}

	// Calls SetMenuItemInfo().
	const menu& set_text_by_cmd(int cmdId, std::wstring_view text) const { return this->_set_text(cmdId, text, false); }
	// Calls SetMenuItemInfo().
	const menu& set_text_by_pos(size_t pos, std::wstring_view text) const { return this->_set_text(static_cast<UINT>(pos), text, true); }

	// Shows the floating menu anchored at the given coordinates with TrackPopupMenu().
	// If hCoordsRelativeTo is null, coordinates must be relative to hParent.
	// This function will block until the menu disappears.
	const menu& show_at_point(
		HWND hParent, POINT pt, HWND hWndCoordsRelativeTo) const
	{
		POINT ptParent = pt; // receives coordinates relative to hParent
		if (!ClientToScreen(hWndCoordsRelativeTo ?
			hWndCoordsRelativeTo : hParent, &ptParent)) // to screen coordinates
		{
			throw std::runtime_error("ClientToScreen failed.");
		}

		SetForegroundWindow(hParent);

		if (!TrackPopupMenu(this->_hMenu, TPM_LEFTBUTTON,
			ptParent.x, ptParent.y, 0, hParent, nullptr)) // owned by hParent, so messages go to it
		{
			throw std::system_error(GetLastError(), std::system_category(),
				"TrackPopupMenu failed.");
		}

		PostMessageW(hParent, WM_NULL, 0, 0); // http://msdn.microsoft.com/en-us/library/ms648002%28VS.85%29.aspx
		return *this;
	}

	// Retrieves the sub menu at the given position.
	[[nodiscard]] menu sub_menu(size_t pos) const
	{
		HMENU hSub = GetSubMenu(this->_hMenu, static_cast<int>(pos));
		if (!hSub) {
			throw std::invalid_argument(
				str::unicode_to_ansi(
					str::format(L"Position %d is not a sub menu.", pos) ));
		}
		return menu{hSub};
	}

	// Retrieves the text with GetMenuItemInfo().
	[[nodiscard]] std::wstring text_by_cmd(int cmdId) const { return this->_text(cmdId, false); }
	// Retrieves the text with GetMenuItemInfo().
	[[nodiscard]] std::wstring text_by_pos(size_t pos) const { return this->_text(static_cast<UINT>(pos), true); }

private:
	const menu& _delete(UINT cmdOrPos, bool byPos) const
	{
		if (!DeleteMenu(this->_hMenu, cmdOrPos, byPos ? MF_BYPOSITION : MF_BYCOMMAND)) {
			throw std::system_error(GetLastError(), std::system_category(),
				str::unicode_to_ansi(
					str::format(L"DeleteMenu %d failed.", cmdOrPos) ));
		}
		return *this;
	}

	const menu& _enable(UINT cmdOrPos, bool isEnabled, bool byPos) const
	{
		UINT flags = (isEnabled ? MF_ENABLED : MF_GRAYED)
			| (byPos ? MF_BYPOSITION : MF_BYCOMMAND);

		if (EnableMenuItem(this->_hMenu, cmdOrPos, flags) == -1) {
			throw std::logic_error(
				str::unicode_to_ansi(
					str::format(L"The menu item %d doesn't exist.", cmdOrPos) ));
		}
		return *this;
	}

	const menu& _insert_item_before(
		UINT cmdOrPosBefore, int newCmdId, std::wstring_view text, bool byPos) const
	{
		UINT flag = byPos ? MF_BYPOSITION : MF_BYCOMMAND;

		if (!InsertMenuW(this->_hMenu, cmdOrPosBefore,
			flag | MF_STRING, newCmdId, text.data() ))
		{
			throw std::system_error(GetLastError(), std::system_category(),
				str::unicode_to_ansi(
					str::format(L"InsertMenu failed for \"%s\".", text) ));
		}
		return *this;
	}

	menu _insert_submenu_before(
		UINT cmdOrPosBefore, std::wstring_view text, bool byPos) const
	{
		UINT flag = byPos ? MF_BYPOSITION : MF_BYCOMMAND;
		HMENU pop = _create_submenu();

		if (!InsertMenuW(this->_hMenu, cmdOrPosBefore, flag | MF_POPUP,
			reinterpret_cast<UINT_PTR>(pop), text.data() ))
		{
			throw std::system_error(GetLastError(), std::system_category(),
				str::unicode_to_ansi(
					str::format(L"InsertMenu failed for \"%s\".", text) ));
		}

		return menu{pop};
	}

	const menu& _set_text(UINT cmdOrPos, std::wstring_view text, bool byPos) const
	{
		MENUITEMINFOW mii{};
		mii.cbSize = sizeof(MENUITEMINFOW);
		mii.fMask = MIIM_STRING;
		mii.dwTypeData = const_cast<wchar_t*>(text.data());

		if (!SetMenuItemInfoW(this->_hMenu, cmdOrPos, byPos, &mii)) {
			throw std::system_error(GetLastError(), std::system_category(),
				str::unicode_to_ansi(
					str::format(L"InsertMenu failed for \"%s\".", text) ));
		}
		return *this;
	}

	[[nodiscard]] const std::wstring _text(UINT cmdOrPos, bool byPos) const
	{
		MENUITEMINFOW mii{};
		mii.cbSize = sizeof(MENUITEMINFOW);
		mii.fMask = MIIM_STRING;

		if (!GetMenuItemInfoW(this->_hMenu, cmdOrPos, byPos, &mii)) { // retrieve length
			throw std::system_error(GetLastError(), std::system_category(),
				str::unicode_to_ansi(
					str::format(L"GetMenuItemInfo failed to retrieve text length from %d.", cmdOrPos) ));
		}
		++mii.cch; // add room for terminating null

		std::wstring buf(mii.cch, L'\0');
		mii.dwTypeData = &buf[0];
		if (!GetMenuItemInfoW(this->_hMenu, cmdOrPos, byPos, &mii)) {
			throw std::system_error(GetLastError(), std::system_category(),
				str::unicode_to_ansi(
					str::format(L"GetMenuItemInfo failed to retrieve text from %d.", cmdOrPos) ));
		}
		return buf;
	}

	[[nodiscard]] static HMENU _create_submenu()
	{
		HMENU pop = CreatePopupMenu();
		if (!pop) {
			throw std::system_error(GetLastError(), std::system_category(),
				"CreatePopupMenu failed.");
		}
		return pop;
	}
};

// Manages a horizontal main window menu.
// Calls CreateMenu() on constructor.
class menu_main final : public menu {
public:
	menu_main() : menu{CreateMenu()}
	{
		if (!this->_hMenu) {
			throw std::system_error(GetLastError(), std::system_category(),
				"CreateMenu failed.");
		}
	}

	menu_main(menu_main&& other) noexcept { this->operator=(std::move(other)); } // movable only

	menu_main& operator=(menu_main&& other) noexcept
	{
		DestroyMenu(this->_hMenu);
		this->_hMenu = nullptr;
		std::swap(this->_hMenu, other._hMenu);
		return *this;
	}
};

// Manages a popup menu.
// Calls CreatePopupMenu() on constructor, DestroyMenu() on destructor.
class menu_popup final : public menu {
public:
	~menu_popup() { this->destroy_menu(); }

	menu_popup() : menu{CreatePopupMenu()}
	{
		if (!this->_hMenu) {
			throw std::system_error(GetLastError(), std::system_category(),
				"CreatePopupMenu failed.");
		}
	}

	menu_popup(menu_popup&& other) noexcept { this->operator=(std::move(other)); } // movable only

	menu_popup& operator=(menu_popup&& other) noexcept
	{
		this->destroy_menu();
		std::swap(this->_hMenu, other._hMenu);
		return *this;
	}

	// Calls DestroyMenu().
	void destroy_menu() noexcept
	{
		if (this->_hMenu) {
			DestroyMenu(this->_hMenu);
			this->_hMenu = nullptr;
		}
	}
};

// Manages a menu loaded from the resource.
// Calls LoadMenu() on constructor.
// Loaded resources are automatically destroyed by the system.
class menu_resource final : public menu {
public:
	menu_resource() = default;
	explicit menu_resource(int menuId) { this->load(menuId); }
	menu_resource(menu_resource&& other) noexcept { this->operator=(std::move(other)); } // movable only

	menu_resource& operator=(menu_resource&& other) noexcept
	{
		this->_hMenu = nullptr;
		std::swap(this->_hMenu, other._hMenu);
		return *this;
	}

	// Calls LoadMenu().
	menu_resource& load(int menuId)
	{
		this->_hMenu = LoadMenuW(GetModuleHandle(nullptr), MAKEINTRESOURCEW(menuId));
		if (!this->_hMenu) {
			throw std::system_error(GetLastError(), std::system_category(),
				"LoadMenu failed.");
		}
		return *this;
	}
};

}//namespace wl