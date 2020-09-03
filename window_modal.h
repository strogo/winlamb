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
#include "internals/base_window.h"
#include "internals/control_visuals.h"
#include "internals/interfaces.h"
#include "internals/str_aux.h"

namespace wl {

// Modal popup window.
// Allows message and notification handling.
class window_modal : public i_window {
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
		// Default: {500, 400}.
		SIZE size = {500, 400};
		// Passed to CreateWindowEx().
		// Default: WS_EX_DLGMODALFRAME.
		DWORD ex_style = WS_EX_DLGMODALFRAME;
		// Passed to CreateWindowEx().
		// Default: WS_CAPTION | WS_SYSMENU | WS_CLIPCHILDREN | WS_BORDER | WS_VISIBLE.
		DWORD style = WS_CAPTION | WS_SYSMENU | WS_CLIPCHILDREN | WS_BORDER | WS_VISIBLE;
	};

	setup_opts _setup;
	_wli::base_window _base;
	HWND _hPrevFocusParent = nullptr;

public:
	window_modal()
	{
		this->_default_msg_handlers();
	}

	window_modal(window_modal&&) = default;
	window_modal& operator=(window_modal&&) = default; // movable only

	// Returns the HWND.
	[[nodiscard]] HWND hwnd() const noexcept override { return this->_base.hwnd(); }

	// Sets the window title.
	const window_modal& set_title(std::wstring_view t) const noexcept { SetWindowTextW(this->hwnd(), t.data()); return *this; }

	// Retrieves the window title.
	[[nodiscard]] std::wstring title() const { return _wli::get_window_text(this->hwnd()); }

	// Exposes variables that will be used in RegisterClassEx() and
	// CreateWindowEx() calls, during window creation.
	[[nodiscard]] setup_opts& setup()
	{
		if (this->hwnd()) {
			throw std::logic_error("Cannot call setup() after window_modal is created.");
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

	// Creates the window and disables the parent.
	// This method will block until the modal is closed.
	void show(const i_window* parent)
	{
		if (!parent) {
			throw std::invalid_argument("No parent passed to window_modal::show().");
		}

		HINSTANCE hInst = reinterpret_cast<HINSTANCE>(
			GetWindowLongPtrW(parent->hwnd(), GWLP_HINSTANCE));
		WNDCLASSEXW wcx = this->_wcx_from_opts(hInst);
		this->_base.register_class(wcx);

		this->_hPrevFocusParent = GetFocus();
		EnableWindow(parent->hwnd(), FALSE); // https://devblogs.microsoft.com/oldnewthing/20040227-00/?p=40463

		this->_setup.size = _wli::multiply_dpi(this->_setup.size);

		HWND h = this->_base.create_window(wcx.hInstance, parent,
			wcx.lpszClassName, this->_setup.title, nullptr, {0, 0}, this->_setup.size,
			this->_setup.ex_style, this->_setup.style);

		RECT rc{}, rcParent{};
		GetWindowRect(h, &rc);
		GetWindowRect(parent->hwnd(), &rcParent); // both relative to screen

		SetWindowPos(h, nullptr, // center modal over parent (warning: happens after WM_CREATE processing)
			rcParent.left + (rcParent.right - rcParent.left) / 2 - (rc.right - rc.left) / 2,
			rcParent.top + (rcParent.bottom - rcParent.top) / 2 - (rc.bottom - rc.top) / 2,
			0, 0, SWP_NOZORDER | SWP_NOSIZE);

		this->_run_modal_loop();
	}

private:
	void _default_msg_handlers()
	{
		this->on_message(WM_CLOSE, [this](param::wm::close) noexcept -> LRESULT
		{
			EnableWindow(GetWindow(this->hwnd(), GW_OWNER), TRUE); // re-enable parent
			DestroyWindow(this->hwnd()); // then destroy modal
			SetFocus(this->_hPrevFocusParent); // this focus could be set on WM_DESTROY as well
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
	}

	void _run_modal_loop()
	{
		for (;;) {
			MSG msg{};
			BOOL gmRet = GetMessageW(&msg, nullptr, 0, 0);
			if (gmRet == -1) {
				throw std::system_error(GetLastError(), std::system_category(),
					"GetMessage failed.");
			} else if (!gmRet) {
				// WM_QUIT was sent, exit modal loop now and signal parent.
				// wParam has the program exit code.
				// https://devblogs.microsoft.com/oldnewthing/20050222-00/?p=36393
				PostQuitMessage(static_cast<int>(msg.wParam));
				break;
			}

			// If a child window, will retrieve its top-level parent.
			// If a top-level, use itself.
			HWND hTopLevel = GetAncestor(msg.hwnd, GA_ROOT);

			if (IsDialogMessageW(hTopLevel, &msg)) {
				// Processed all keyboard actions for child controls.
				if (!this->hwnd()) {
					break; // our modal was destroyed, terminate loop
				} else {
					continue;
				}
			}

			TranslateMessage(&msg);
			DispatchMessageW(&msg);

			if (!this->hwnd()) {
				break; // our modal was destroyed, terminate loop
			}
		}
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