/**
 * Part of WinLamb - Win32 API Lambda Library
 * https://github.com/rodrigocfd/winlamb
 * This library is released under the MIT License
 */

#pragma once
#include <stdexcept>
#include <string_view>
#include <Windows.h>
#include "internals/base_window.h"
#include "internals/control_visuals.h"
#include "internals/interfaces.h"

namespace wl {

// Custom user control.
// Allows message and notification handling.
class window_control : public i_control {
private:
	struct setup_opts final {
		// Passed in WNDCLASSEX to RegisterClassEx().
		// Default: auto-generated string.
		std::wstring class_name;
		// Passed in WNDCLASSEX to RegisterClassEx().
		// Default: CS_DBLCLKS.
		DWORD class_style = CS_DBLCLKS;
		// Passed in WNDCLASSEX to RegisterClassEx().
		// Window HCURSOR, default: IDC_ARROW.
		HCURSOR class_cursor = nullptr;
		// Passed in WNDCLASSEX to RegisterClassEx().
		// Window background HBRUSH, default: COLOR_WINDOW (white).
		HBRUSH class_bg_brush = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);

		// Passed to CreateWindowEx().
		// Default: zero, use WS_EX_CLIENTEDGE for a border.
		DWORD ex_style = 0;
		// Passed to CreateWindowEx().
		// Default: WS_CHILD | WS_TABSTOP | WS_GROUP | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS.
		DWORD style = WS_CHILD | WS_TABSTOP | WS_GROUP | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
	};

	setup_opts _setup;
	_wli::base_window _base;

public:
	window_control()
	{
		this->_default_msg_handlers();
	}

	window_control(window_control&&) = default;
	window_control& operator=(window_control&&) = default; // movable only

	// Returns the HWND.
	[[nodiscard]] HWND hwnd() const noexcept override { return this->_base.hwnd(); }

	// Retrieves the control ID.
	[[nodiscard]] int id() const noexcept override { return GetDlgCtrlID(this->hwnd()); }

	// Exposes variables that will be used in RegisterClassEx() and
	// CreateWindowEx() calls, during window creation.
	[[nodiscard]] setup_opts& setup()
	{
		if (this->hwnd()) {
			throw std::logic_error("Cannot call setup() after window_control is created.");
		}
		return this->_setup;
	}

	// Adds a lambda to handle any window message.
	// func: [](wl::param::any p) -> LRESULT { }
	template<typename F>
	void on_message(UINT msg, F&& func) { this->_base.on_message(msg, std::move(func)); }

	// Adds a lambda to handle any window message.
	// func: [](wl::param::any p) -> LRESULT { }
	template<typename F>
	void on_message(std::initializer_list<UINT> msgs, F&& func) { this->_base.on_message(msgs, std::move(func)); }

	// Adds a lambda to handle a WM_COMMAND message for a specific command ID.
	// func: [](wl::param::any p) -> LRESULT { }
	template<typename F>
	void on_command(WORD cmd, F&& func) { this->_base.on_command(cmd, std::move(func)); }

	// Adds a lambda to handle a WM_COMMAND message for a specific command ID.
	// func: [](wl::param::any p) -> LRESULT { }
	template<typename F>
	void on_command(std::initializer_list<WORD> cmds, F&& func) { this->_base.on_command(cmds, std::move(func)); }

	// Adds a lambda to handle a window notification for specific idFrom and code.
	// func: [](wl::param::any p) -> LRESULT { }
	template<typename F>
	void on_notify(UINT_PTR idFrom, int code, F&& func) { this->_base.on_notify(idFrom, code, std::move(func)); }

	// Adds a lambda to handle a window notification for specific idFrom and code.
	// func: [](wl::param::any p) -> LRESULT { }
	template<typename F>
	void on_notify(std::initializer_list<std::pair<UINT_PTR, int>> idFromAndCodes, F&& func) { this->_base.on_notify(idFromAndCodes, std::move(func)); }

	// Runs code synchronously in the UI thread, useful to update
	// the UI within wl::run_thread_detached([](){ ... }).
	// This function will block until it returns.
	// func: []() { }
	template<typename F>
	void run_thread_ui(F&& func) { this->_base.run_thread_ui(std::move(func)); }

	// Creates the control.
	// Should be called during parent's WM_CREATE processing.
	// Position and size will be adjusted to match current system DPI.
	void create(const i_window* parent, int id, POINT pos, SIZE size)
	{
		if (!parent) {
			throw std::invalid_argument("No parent passed to window_control::create().");
		}

		HINSTANCE hInst = reinterpret_cast<HINSTANCE>(
			GetWindowLongPtrW(parent->hwnd(), GWLP_HINSTANCE));
		WNDCLASSEXW wcx = this->_wcx_from_opts(hInst);
		this->_base.register_class(wcx);

		pos = _wli::multiply_dpi(pos);
		size = _wli::multiply_dpi(size);

		this->_base.create_window(wcx.hInstance, parent, wcx.lpszClassName, {},
			reinterpret_cast<HMENU>(static_cast<LONG_PTR>(id)),
			pos, size, this->_setup.ex_style, this->_setup.style);
	}

private:
	void _default_msg_handlers()
	{
		this->on_message(WM_NCPAINT, [this](param::wm::ncpaint p) noexcept -> LRESULT
		{
			_wli::paint_control_borders(this->hwnd(), p.wparam, p.lparam);
			return 0;
		});
	}

	[[nodiscard]] WNDCLASSEXW _wcx_from_opts(HINSTANCE hInst)
	{
		WNDCLASSEXW wcx{};
		wcx.cbSize = sizeof(WNDCLASSEXW);
		wcx.hInstance = hInst;
		wcx.style = this->_setup.class_style;
		wcx.hbrBackground = this->_setup.class_bg_brush;

		_wli::base_window::wcx_set_cursor(this->_setup.class_cursor, wcx);

		if (this->_setup.class_name.empty()) { // if user didn't choose a class name
			this->_setup.class_name = _wli::base_window::wcx_generate_hash(wcx); // generate hash after all fields are set
			wcx.lpszClassName = this->_setup.class_name.c_str();
		}
		return wcx;
	}
};

}//namespace wl