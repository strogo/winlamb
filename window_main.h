/**
 * Part of WinLamb - Win32 API Lambda Library
 * https://github.com/rodrigocfd/winlamb
 * This library is released under the MIT License
 */

#pragma once
#include <stdexcept>
#include <string_view>
#include <vector>
#include <Windows.h>
#include <CommCtrl.h>
#include <VersionHelpers.h>
#include "internals/base_main_loop.h"
#include "internals/base_window.h"
#include "internals/control_visuals.h"
#include "internals/gdi_obj.h"
#include "internals/interfaces.h"
#include "internals/str_aux.h"
#include "accel_table.h"
#include "menu.h"

namespace wl {

// Main application window.
// Allows message and notification handling.
class window_main : public i_window {
private:
	struct setup_opts final {
		// Passed in WNDCLASSEX to RegisterClassEx().
		// Default: auto-generated string.
		std::wstring class_name;
		// Passed in WNDCLASSEX to RegisterClassEx().
		// Default: CS_DBLCLKS.
		DWORD class_style = CS_DBLCLKS;
		// Passed in WNDCLASSEX to RegisterClassEx().
		// Window main HICON, default: none;
		HICON class_icon = nullptr;
		// Passed in WNDCLASSEX to RegisterClassEx().
		// Window small HICON, default: none;
		HICON class_icon_sm = nullptr;
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
		// Default: {600, 500}.
		SIZE size = {600, 500};
		// Passed to CreateWindowEx().
		// Default: zero.
		DWORD ex_style = 0;
		// Passed to CreateWindowEx().
		// Default: WS_CAPTION | WS_SYSMENU | WS_CLIPCHILDREN | WS_BORDER | WS_VISIBLE.
		// Suggestions: WS_SIZEBOX (resizable), WS_MINIMIZEBOX, WS_MAXIMIZEBOX.
		DWORD style = WS_CAPTION | WS_SYSMENU | WS_CLIPCHILDREN | WS_BORDER;
		// Passed to CreateWindowEx().
		// Main window menu. This menu is not shared, window_main owns it.
		// Default: empty.
		menu_main main_menu;

		// Accelerator table for keyboard shortcuts.
		// Default: empty.
		accel_table accel_tbl;
	};

	setup_opts _setup;
	_wli::base_window _base;
	_wli::base_main_loop _mainLoop;
	HWND _hChildPrevFocus = nullptr; // WM_ACTIVATE woes

public:
	window_main()
	{
		this->_default_msg_handlers();
	}

	window_main(window_main&&) = default;
	window_main& operator=(window_main&&) = default; // movable only

	// Returns the HWND.
	[[nodiscard]] HWND hwnd() const noexcept override { return this->_base.hwnd(); }

	// Sets the window title.
	const window_main& set_title(std::wstring_view t) const noexcept { SetWindowTextW(this->hwnd(), t.data()); return *this; }

	// Retrieves the window title.
	[[nodiscard]] std::wstring title() const { return _wli::get_window_text(this->hwnd()); }

	// Returns the horizontal main window menu.
	[[nodiscard]] const menu& main_menu() const noexcept { return this->_setup.main_menu; }

	// Exposes variables that will be used in RegisterClassEx() and
	// CreateWindowEx() calls, during window creation.
	[[nodiscard]] setup_opts& setup()
	{
		if (this->hwnd()) {
			throw std::logic_error("Cannot call setup() after window_main is created.");
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

	// Creates the window and runs the main application loop.
	int run_as_main(HINSTANCE hInst, int cmdShow)
	{
		if (IsWindowsVistaOrGreater()) {
			SetProcessDPIAware();
		}
		InitCommonControls();
		_wli::globalUiFont.create_ui();

		WNDCLASSEXW wcx = this->_wcx_from_opts(hInst);
		this->_base.register_class(wcx);

		this->_setup.size = _wli::multiply_dpi(this->_setup.size);

		SIZE szScreen = {GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN)};
		POINT pos = {
			szScreen.cx / 2 - this->_setup.size.cx / 2, // center on screen
			szScreen.cy / 2 - this->_setup.size.cy / 2
		};

		HWND h = this->_base.create_window(wcx.hInstance, nullptr,
			wcx.lpszClassName, this->_setup.title, this->_setup.main_menu.hmenu(),
			pos, this->_setup.size, this->_setup.ex_style, this->_setup.style);
		ShowWindow(h, cmdShow);

		if (!UpdateWindow(h)) {
			throw std::runtime_error("UpdateWindow failed when creating window_main.");
		}
		return this->_mainLoop.run_loop(
			this->hwnd(), this->_setup.accel_tbl.haccel());
	}

private:
	void _default_msg_handlers()
	{
		this->on_message(WM_NCDESTROY, [](param::wm::ncdestroy) noexcept -> LRESULT
		{
			PostQuitMessage(0);
			return 0;
		});

		this->on_message(WM_SETFOCUS, [this](param::wm::setfocus) noexcept -> LRESULT
		{
			if (this->hwnd() == GetFocus()) {
				// If window receives focus, delegate to first child.
				SetFocus(GetNextDlgTabItem(this->hwnd(), nullptr, FALSE));
			}
			return 0;
		});

		this->on_message(WM_ACTIVATE, [this](param::wm::activate p) noexcept -> LRESULT
		{
			if (!p.is_minimized()) {
				if (!p.is_being_activated()) {
					HWND hCurFocus = GetFocus();
					if (hCurFocus && IsChild(this->hwnd(), hCurFocus)) {
						this->_hChildPrevFocus = hCurFocus; // save previously focused control
					}
				} else if (this->_hChildPrevFocus) {
					SetFocus(this->_hChildPrevFocus); // put focus back
				}
			}
			return 0;
		});

		this->on_message(_wli::WM_MODELESS_CREATED, [this](param::any p) -> LRESULT
		{
			if (p.wparam == 0xc0def00d) {
				this->_mainLoop.add_modeless_child(
					reinterpret_cast<HWND>(p.lparam));
			}
			return 0;
		});

		this->on_message(_wli::WM_MODELESS_DESTROYED, [this](param::any p) -> LRESULT
		{
			if (p.wparam == 0xc0def00d) {
				this->_mainLoop.delete_modeless_child(
					reinterpret_cast<HWND>(p.lparam));
			}
			return 0;
		});
	}

	[[nodiscard]] WNDCLASSEXW _wcx_from_opts(HINSTANCE hInst)
	{
		WNDCLASSEXW wcx{};
		wcx.cbSize = sizeof(WNDCLASSEXW);
		wcx.hInstance = hInst;
		wcx.style = this->_setup.class_style;
		wcx.hIcon = this->_setup.class_icon;
		wcx.hIconSm = this->_setup.class_icon_sm;
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