/**
 * Part of WinLamb - Win32 API Lambda Library
 * https://github.com/rodrigocfd/winlamb
 * This library is released under the MIT License
 */

#pragma once
#include <string_view>
#include <Windows.h>
#include "internals/base_native_control.h"
#include "internals/control_visuals.h"
#include "internals/gdi_obj.h"
#include "internals/interfaces.h"

namespace wl {

// Native check box control.
// https://docs.microsoft.com/en-us/windows/win32/controls/button-types-and-styles#check-boxes
class check_box final : public i_control {
private:
	_wli::base_native_control _base;

public:
	check_box() = default;
	check_box(check_box&&) = default;
	check_box& operator=(check_box&&) = default; // movable only

	// Returns the HWND.
	[[nodiscard]] HWND hwnd() const noexcept override { return this->_base.hwnd(); }

	// Retrieves the control ID.
	[[nodiscard]] int id() const noexcept override { return this->_base.id(); }

	// Calls EnableWindow().
	const check_box& enable(bool isEnabled) const noexcept { EnableWindow(this->hwnd(), isEnabled); return *this; }

	// In a dialog window, assigns to a control.
	check_box& assign(const i_window* parent, int ctrlId) { this->_base.assign(parent, ctrlId); return *this; }

	// Installs a window subclass and adds a handle to a message.
	// func: [](wl::param::any p) -> LRESULT { }
	template<typename F>
	void on_subclass_message(UINT msg, F&& func) { this->_base.on_subclass_message(msg, std::move(func)); }

	// Installs a window subclass and adds a handle to a message.
	// func: [](wl::param::any p) -> LRESULT { }
	template<typename F>
	void on_subclass_message(std::initializer_list<UINT> msgs, F&& func) { this->_base.on_subclass_message(msgs, std::move(func)); }

	// Retrieves the text in this control.
	[[nodiscard]] std::wstring text() const { return _wli::get_window_text(this->hwnd()); }

	// Calls CreateWindowEx().
	// Should be called during parent's WM_CREATE processing.
	// Position will be adjusted to match current system DPI.
	check_box& create(const i_window* parent, int id,
		std::wstring_view text, POINT pos)
	{
		pos = _wli::multiply_dpi(pos);
		SIZE size = _wli::calc_check_bound_box(parent->hwnd(), text);

		HWND h = this->_base.create_window(parent, id, L"BUTTON", text, pos, size,
			WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_GROUP | BS_AUTOCHECKBOX, 0);

		_wli::globalUiFont.set_on_control(*this);
		return *this;
	}

	// Tells if current state is BST_CHECKED.
	[[nodiscard]] bool checked() const noexcept
	{
		return SendMessageW(this->hwnd(), BM_GETCHECK, 0, 0) == BST_CHECKED;
	}

	// Sets the state to BST_CHECKED or BST_UNCHECKED.
	const check_box& set_checked(bool isChecked) const noexcept
	{
		SendMessageW(this->hwnd(), BM_SETCHECK,
			isChecked ? BST_CHECKED : BST_UNCHECKED, 0);
		return *this;
	}

	// Calls set_checked() and sends a WM_COMMAND message emulating the user click.
	const check_box& set_checked_and_trigger(bool isChecked) const noexcept
	{
		this->set_checked(isChecked);
		SendMessageW(GetParent(this->hwnd()), WM_COMMAND,
			MAKEWPARAM(this->id(), 0),
			reinterpret_cast<LPARAM>(this->hwnd()) );
		return *this;
	}

	// Sets the text in the control, and resizes it to fit.
	const check_box& set_text(std::wstring_view text) const
	{
		SIZE newSize = _wli::calc_check_bound_box(GetParent(this->hwnd()), text);
		SetWindowPos(this->hwnd(), nullptr, 0, 0,
			newSize.cx, newSize.cy, SWP_NOZORDER | SWP_NOMOVE);

		SetWindowTextW(this->hwnd(), text.data());
		return *this;
	}
};

}//namespace wl