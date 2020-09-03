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
#include <CommCtrl.h>
#include <VersionHelpers.h>
#include "internals/base_dialog.h"
#include "internals/base_main_loop.h"
#include "internals/gdi_obj.h"
#include "internals/interfaces.h"
#include "internals/str_aux.h"

namespace wl {

class dialog_main : public i_window {
private:
	struct setup_opts final {
		// Passed to CreateDialogParam().
		// Resource dialog ID, must be set.
		int dialog_id = 0;
		// Resource icon ID, optional.
		int icon_id = 0;
		// Resource accelerator table ID, optional.
		int accel_tbl_id = 0;
	};

	setup_opts _setup;
	_wli::base_dialog _base;
	_wli::base_main_loop _mainLoop;

public:
	dialog_main()
	{
		this->_default_msg_handlers();
	}

	dialog_main(dialog_main&&) = default;
	dialog_main& operator=(dialog_main&&) = default; // movable only

	// Returns the HWND.
	[[nodiscard]] HWND hwnd() const noexcept override { return this->_base.hwnd(); }

	// Sets the window title.
	const dialog_main& set_title(std::wstring_view t) const noexcept { SetWindowTextW(this->hwnd(), t.data()); return *this; }

	// Retrieves the window title.
	[[nodiscard]] std::wstring title() const { return _wli::get_window_text(this->hwnd()); }

	// Exposes variables that will be used in RegisterClassEx() and
	// CreateWindowEx() calls, during window creation.
	[[nodiscard]] setup_opts& setup()
	{
		if (this->hwnd()) {
			throw std::logic_error("Cannot call setup() after dialog_main is created.");
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
	// func: [](wl::param::any p) -> INT_PTR { }
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

		this->_base.create_dialog_param(hInst, nullptr, this->_setup.dialog_id);

		HACCEL hAccel = nullptr;
		if (this->_setup.accel_tbl_id) {
			// An accelerator table loaded from resource is automatically freed by the system.
			hAccel = LoadAcceleratorsW(hInst, MAKEINTRESOURCEW(this->_setup.accel_tbl_id));
			if (!hAccel) {
				throw std::system_error(GetLastError(), std::system_category(),
					"LoadAccelerators failed for main dialog.");
			}
		}

		this->_set_icon_if_any(hInst);
		ShowWindow(this->hwnd(), cmdShow);
		return this->_mainLoop.run_loop(this->hwnd(), hAccel);
	}

private:
	void _default_msg_handlers()
	{
		this->on_message(WM_NCDESTROY, [](param::wm::ncdestroy) noexcept -> INT_PTR
		{
			PostQuitMessage(0);
			return TRUE;
		});

		this->on_message(WM_CLOSE, [this](param::wm::close) noexcept -> INT_PTR
		{
			DestroyWindow(this->hwnd());
			return TRUE;
		});
	}

	void _set_icon_if_any(HINSTANCE hInst) const noexcept
	{
		// If an icon ID was specified, load it from the resources.
		// Resource icons are automatically released by the system.
		if (this->_setup.icon_id) {
			SendMessageW(this->hwnd(), WM_SETICON, ICON_SMALL,
				reinterpret_cast<LPARAM>(reinterpret_cast<HICON>(LoadImageW(hInst,
					MAKEINTRESOURCEW(this->_setup.icon_id),
					IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR))));

			SendMessageW(this->hwnd(), WM_SETICON, ICON_BIG,
				reinterpret_cast<LPARAM>(reinterpret_cast<HICON>(LoadImageW(hInst,
					MAKEINTRESOURCEW(this->_setup.icon_id),
					IMAGE_ICON, 32, 32, LR_DEFAULTCOLOR))));
		}
	}
};

}//namespace wl