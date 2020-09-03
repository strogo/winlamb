/**
 * Part of WinLamb - Win32 API Lambda Library
 * https://github.com/rodrigocfd/winlamb
 * This library is released under the MIT License
 */

#pragma once
#include <optional>
#include <string_view>
#include <Windows.h>
#include "internals/base_native_control.h"
#include "internals/control_visuals.h"
#include "internals/gdi_obj.h"
#include "internals/interfaces.h"

namespace wl {

// Native edit (textbox) control.
// https://docs.microsoft.com/en-us/windows/win32/controls/about-edit-controls
class edit : public i_control {
private:
	_wli::base_native_control _base;

public:
	enum class type {
		// Single line, ES_AUTOHSCROLL.
		NORMAL,
		// Single line, ES_AUTOHSCROLL, ES_PASSWORD.
		PASSWORD,
		// Multi line, ES_MULTILINE, ES_WANTRETURN.
		MULTILINE
	};

	edit() = default;
	edit(edit&&) = default;
	edit& operator=(edit&&) = default; // movable only

	// Returns the HWND.
	[[nodiscard]] HWND hwnd() const noexcept override { return this->_base.hwnd(); }

	// Retrieves the control ID.
	[[nodiscard]] int id() const noexcept override { return this->_base.id(); }

	// Calls EnableWindow().
	const edit& enable(bool isEnabled) const noexcept { EnableWindow(this->hwnd(), isEnabled); return *this; }

	// In a dialog window, assigns to a control.
	edit& assign(const i_window* parent, int ctrlId) { this->_base.assign(parent, ctrlId); return *this; }

	// Installs a window subclass and adds a handle to a message.
	// func: [](wl::param::any p) -> LRESULT { }
	template<typename F>
	void on_subclass_message(UINT msg, F&& func) { this->_base.on_subclass_message(msg, std::move(func)); }

	// Installs a window subclass and adds a handle to a message.
	// func: [](wl::param::any p) -> LRESULT { }
	template<typename F>
	void on_subclass_message(std::initializer_list<UINT> msgs, F&& func) { this->_base.on_subclass_message(msgs, std::move(func)); }

	// Sets the text in this control.
	const edit& set_text(std::wstring_view t) const noexcept { SetWindowTextW(this->hwnd(), t.data()); return *this; }

	// Retrieves the text in this control.
	[[nodiscard]] std::wstring text() const { return _wli::get_window_text(this->hwnd()); }

	// Calls CreateWindowEx().
	// Should be called during parent's WM_CREATE processing.
	// Position and size will be adjusted to match current system DPI.
	edit& create(const i_window* parent, int id, type t,
		std::optional<std::wstring_view> text, POINT pos, SIZE size = {100, 21})
	{
		DWORD styles = WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_GROUP;
		switch (t) {
			case type::NORMAL:    styles |= ES_AUTOHSCROLL; break;
			case type::PASSWORD:  styles |= ES_AUTOHSCROLL | ES_PASSWORD; break;
			case type::MULTILINE: styles |= ES_MULTILINE | ES_WANTRETURN;
		}

		pos = _wli::multiply_dpi(pos);
		size = _wli::multiply_dpi(size);

		HWND h = this->_base.create_window(parent, id, L"EDIT", text,
			pos, size, styles, WS_EX_CLIENTEDGE);

		_wli::globalUiFont.set_on_control(*this);
		return *this;
	}

	// Replaces the currently selected text with the given one.
	const edit& replace_selection(std::wstring_view replacement) const noexcept
	{
		SendMessageW(this->hwnd(), EM_REPLACESEL, TRUE,
			reinterpret_cast<LPARAM>(replacement.data()));
		return *this;
	}

	// Selects all the text.
	const edit& select_all() const noexcept
	{
		SendMessageW(this->hwnd(), EM_SETSEL, 0, -1);
		return *this;
	}

	// Returns the selected text.
	[[nodiscard]] std::wstring selection() const
	{
		DWORD start = 0, pastEnd = 0;
		SendMessageW(this->hwnd(), EM_GETSEL,
			reinterpret_cast<WPARAM>(&start), reinterpret_cast<LPARAM>(&pastEnd));
		std::wstring text = this->text();
		return text.substr(start, pastEnd - start);
	}

	// Sets the selection range.
	const edit& set_selection(int start, int length) const noexcept
	{
		SendMessageW(this->hwnd(), EM_SETSEL, start, static_cast<size_t>(start) + length);
		return *this;
	}
};

}//namespace wl