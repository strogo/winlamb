/**
 * Part of WinLamb - Win32 API Lambda Library
 * https://github.com/rodrigocfd/winlamb
 * This library is released under the MIT License
 */

#pragma once
#include <stdexcept>
#include <string_view>
#include <Windows.h>
#include "internals/base_main_loop.h"
#include "internals/base_window.h"
#include "internals/control_visuals.h"
#include "internals/interfaces.h"

namespace wl {

// Modeless popup window.
// Allows message and notification handling.
class window_modeless : public i_window {
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
		// Window background HBRUSH, default: COLOR_BTNFACE (brown/gray).
		HBRUSH class_bg_brush = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);

		// Passed to CreateWindowEx().
		// Window title, default: empty string.
		std::wstring title;
		// Passed to CreateWindowEx().
		// Default: {300, 200}.
		SIZE size = {300, 200};
		// Passed to CreateWindowEx().
		// Default: WS_EX_TOOLWINDOW.
		DWORD ex_style = WS_EX_TOOLWINDOW;
		// Passed to CreateWindowEx().
		// Default: WS_CAPTION | WS_CLIPCHILDREN | WS_BORDER | WS_VISIBLE.
		// Suggestion: WS_SYSMENU (X button).
		DWORD style = WS_CAPTION | WS_CLIPCHILDREN | WS_BORDER | WS_VISIBLE;
	};

	setup_opts _setup;
	_wli::base_window _base;

public:
	window_modeless()
	{
		this->_default_msg_handlers();
	}

	window_modeless(window_modeless&&) = default;
	window_modeless& operator=(window_modeless&&) = default; // movable only

	// Returns the HWND.
	[[nodiscard]] HWND hwnd() const noexcept override { return this->_base.hwnd(); }

	// Sets the window title.
	const window_modeless& set_title(std::wstring_view t) const noexcept { SetWindowTextW(this->hwnd(), t.data()); return *this; }

	// Retrieves the window title.
	[[nodiscard]] std::wstring title() const { return _wli::get_window_text(this->hwnd()); }

	// Exposes variables that will be used in RegisterClassEx() and
	// CreateWindowEx() calls, during window creation.
	[[nodiscard]] setup_opts& setup()
	{
		if (this->hwnd()) {
			throw std::logic_error("Cannot call setup() after window_modeless is created.");
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

	// Creates the window and returns immediately.
	void create(const i_window* parent)
	{
		if (!parent) {
			throw std::invalid_argument("No parent passed to window_modeless::create().");
		}

		HINSTANCE hInst = reinterpret_cast<HINSTANCE>(
			GetWindowLongPtrW(parent->hwnd(), GWLP_HINSTANCE));
		WNDCLASSEXW wcx = this->_wcx_from_opts(hInst);
		this->_base.register_class(wcx);

		this->_setup.size = _wli::multiply_dpi(this->_setup.size);

		HWND h = this->_base.create_window(wcx.hInstance, parent,
			wcx.lpszClassName, this->_setup.title, nullptr, {0, 0}, this->_setup.size,
			this->_setup.ex_style, this->_setup.style);

		SendMessageW(parent->hwnd(), _wli::WM_MODELESS_CREATED, // tell parent we're here
			0xc0def00d, reinterpret_cast<LPARAM>(h) );

		RECT rc{}, rcParent{};
		GetWindowRect(h, &rc);
		GetWindowRect(parent->hwnd(), &rcParent); // both relative to screen

		SetWindowPos(h, nullptr, // place over parent (warning: happens after WM_CREATE processing)
			rcParent.right - (rc.right - rc.left),
			rcParent.top + 34,
			0, 0, SWP_NOZORDER | SWP_NOSIZE);
	}

private:
	void _default_msg_handlers()
	{
		this->on_message(WM_CLOSE, [this](param::wm::close) noexcept -> LRESULT
		{
			DestroyWindow(this->hwnd());
			return 0;
		});

		this->on_message(WM_NCDESTROY, [this](param::wm::ncdestroy) noexcept -> LRESULT
		{
			SendMessageW(GetWindow(this->hwnd(), GW_OWNER), // tell parent we're gone
				_wli::WM_MODELESS_DESTROYED,
				0xc0def00d, reinterpret_cast<LPARAM>(this->hwnd()) );
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