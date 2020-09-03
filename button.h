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

// Native button control.
// https://docs.microsoft.com/en-us/windows/win32/controls/button-types-and-styles#push-buttons
class button final : public i_control {
private:
	_wli::base_native_control _base;

public:
	enum class type {
		// Button will not have BS_DEFPUSHBUTTON.
		NORMAL,
		// Button will have BS_DEFPUSHBUTTON.
		DEFPUSH
	};

	button() = default;
	button(button&&) = default;
	button& operator=(button&&) = default; // movable only

	// Returns the HWND.
	[[nodiscard]] HWND hwnd() const noexcept override { return this->_base.hwnd(); }

	// Retrieves the control ID.
	[[nodiscard]] int id() const noexcept override { return this->_base.id(); }

	// Calls EnableWindow().
	const button& enable(bool isEnabled) const noexcept { EnableWindow(this->hwnd(), isEnabled); return *this; }

	// In a dialog window, assigns to a control.
	button& assign(const i_window* parent, int ctrlId) { this->_base.assign(parent, ctrlId); return *this; }

	// Installs a window subclass and adds a handle to a message.
	// func: [](wl::param::any p) -> LRESULT { }
	template<typename F>
	void on_subclass_message(UINT msg, F&& func) { this->_base.on_subclass_message(msg, std::move(func)); }

	// Installs a window subclass and adds a handle to a message.
	// func: [](wl::param::any p) -> LRESULT { }
	template<typename F>
	void on_subclass_message(std::initializer_list<UINT> msgs, F&& func) { this->_base.on_subclass_message(msgs, std::move(func)); }

	// Sets the text in this control.
	const button& set_text(std::wstring_view t) const noexcept { SetWindowTextW(this->hwnd(), t.data()); return *this; }

	// Retrieves the text in this control.
	[[nodiscard]] std::wstring text() const { return _wli::get_window_text(this->hwnd()); }

	// Calls CreateWindowEx().
	// Should be called during parent's WM_CREATE processing.
	// Position and size will be adjusted to match current system DPI.
	button& create(const i_window* parent, int id, type t,
		std::wstring_view text, POINT pos, SIZE size = {80, 23})
	{
		DWORD styles = WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_GROUP
			| (t == type::DEFPUSH ? BS_DEFPUSHBUTTON : 0);

		pos = _wli::multiply_dpi(pos);
		size = _wli::multiply_dpi(size);

		HWND h = this->_base.create_window(parent, id, L"BUTTON", text,
			pos, size, styles, 0);

		_wli::globalUiFont.set_on_control(*this);
		return *this;
	}
};

}//namespace wl