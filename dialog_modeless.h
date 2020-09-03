/**
 * Part of WinLamb - Win32 API Lambda Library
 * https://github.com/rodrigocfd/winlamb
 * This library is released under the MIT License
 */

#pragma once
#include <string_view>
#include <Windows.h>
#include "internals/base_dialog.h"
#include "internals/base_main_loop.h"
#include "internals/interfaces.h"

namespace wl {

// Modeless popup dialog.
// Allows message and notification handling.
class dialog_modeless : public i_window {
public:
	struct setup_opts final {
		// Passed to DialogBoxParam().
		// Resource dialog ID, must be set.
		int dialog_id = 0;
	};

	setup_opts _setup;
	_wli::base_dialog _base;

public:
	dialog_modeless()
	{
		this->_default_msg_handlers();
	}

	dialog_modeless(dialog_modeless&&) = default;
	dialog_modeless& operator=(dialog_modeless&&) = default; // movable only

	// Returns the HWND.
	[[nodiscard]] HWND hwnd() const noexcept override { return this->_base.hwnd(); }

	// Sets the window title.
	const dialog_modeless& set_title(std::wstring_view t) const noexcept { SetWindowTextW(this->hwnd(), t.data()); return *this; }

	// Retrieves the window title.
	[[nodiscard]] std::wstring title() const { return _wli::get_window_text(this->hwnd()); }

	// Exposes variables that will be used in RegisterClassEx() and
	// CreateWindowEx() calls, during window creation.
	setup_opts& setup()
	{
		if (this->hwnd()) {
			throw std::logic_error("Cannot call setup() after dialog_modeless is created.");
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

	// Creates the window and returns immediately.
	void create(const i_window* parent)
	{
		if (!parent) {
			throw std::invalid_argument("No parent passed to dialog_modeless::create().");
		}

		HINSTANCE hInst = reinterpret_cast<HINSTANCE>(
			GetWindowLongPtrW(parent->hwnd(), GWLP_HINSTANCE));
		HWND h = this->_base.create_dialog_param(hInst, nullptr, this->_setup.dialog_id);

		SendMessageW(parent->hwnd(), _wli::WM_MODELESS_CREATED, // tell parent we're here
			0xc0def00d, reinterpret_cast<LPARAM>(h) );
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
};

}//namespace wl