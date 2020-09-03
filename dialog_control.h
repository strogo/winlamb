/**
 * Part of WinLamb - Win32 API Lambda Library
 * https://github.com/rodrigocfd/winlamb
 * This library is released under the MIT License
 */

#pragma once
#include <Windows.h>
#include "internals/base_dialog.h"
#include "internals/control_visuals.h"
#include "internals/interfaces.h"

namespace wl {

// Custom dialog-based user control.
// Allows message and notification handling.
// Dialog styles to be set on the resource editor:
// - Border: none
// - Control: true
// - Style: child
// - Visible: true (otherwise will start invisible)
// - Client Edge: true (only if you want a border)
class dialog_control : public i_control {
private:
	struct setup_opts final {
		// Passed to CreateDialogParam().
		// Resource dialog ID, must be set.
		int dialog_id = 0;
	};

	setup_opts _setup;
	_wli::base_dialog _base;

public:
	dialog_control()
	{
		this->_default_msg_handlers();
	}

	dialog_control(dialog_control&&) = default;
	dialog_control& operator=(dialog_control&&) = default; // movable only

	// Returns the HWND.
	[[nodiscard]] HWND hwnd() const noexcept override { return this->_base.hwnd(); }

	// Retrieves the control ID.
	[[nodiscard]] int id() const noexcept override { return GetDlgCtrlID(this->hwnd()); }

	// Exposes variables that will be used in RegisterClassEx() and
	// CreateWindowEx() calls, during window creation.
	[[nodiscard]] setup_opts& setup()
	{
		if (this->hwnd()) {
			throw std::logic_error("Cannot call setup() after dialog_control is created.");
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

	// Creates the control.
	// Should be called during parent's WM_CREATE processing.
	// A size of {0, 0} will use the size defined in the dialog resource.
	// Position and size will be adjusted to match current system DPI.
	void create(const i_window* parent, int id, POINT pos, SIZE size = {0, 0})
	{
		if (!parent) {
			throw std::invalid_argument("No parent passed to dialog_control::create().");
		}

		HINSTANCE hInst = reinterpret_cast<HINSTANCE>(
			GetWindowLongPtrW(parent->hwnd(), GWLP_HINSTANCE));
		this->_base.create_dialog_param(hInst, parent, this->_setup.dialog_id);

		SetWindowLongPtrW(this->hwnd(), GWLP_ID, id); // so the control has an ID

		bool useResourceSize = !size.cx && !size.cy;
		SetWindowPos(this->hwnd(), nullptr, pos.x, pos.y, size.cx, size.cy,
			SWP_NOZORDER | (useResourceSize ? SWP_NOSIZE : 0));
	}

private:
	void _default_msg_handlers()
	{
		this->on_message(WM_NCPAINT, [this](param::wm::ncpaint p) noexcept -> INT_PTR
		{
			_wli::paint_control_borders(this->hwnd(), p.wparam, p.lparam);
			return TRUE;
		});
	}
};

}//namespace wl