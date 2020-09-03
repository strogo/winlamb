/**
 * Part of WinLamb - Win32 API Lambda Library
 * https://github.com/rodrigocfd/winlamb
 * This library is released under the MIT License
 */

#pragma once
#include <optional>
#include <stdexcept>
#include <system_error>
#include <Windows.h>
#include "base_msg_handler.h"
#include "gdi_obj.h"
#include "interfaces.h"

// Owns the HWND.
// Calls CreateDialogParam() or DialogBoxParam().
// Provides the dialog procedure.
namespace _wli {

class base_dialog final {
private:
	HWND _hWnd = nullptr;
	base_msg_handler _msgHandler;

public:
	~base_dialog()
	{
		if (this->_hWnd) {
			SetWindowLongPtrW(this->_hWnd, DWLP_USER, 0); // clear passed pointer
		}
	}

	[[nodiscard]] const HWND& hwnd() const noexcept { return this->_hWnd; } // note: returning a const reference

	template<typename F>
	void run_thread_ui(F&& func) { this->_msgHandler.run_thread_ui(this->_hWnd, std::move(func)); }

	template<typename F>
	void on_message(UINT msg, F&& func)
	{
		if (this->_hWnd) {
			throw std::logic_error("Cannot add a message handler after the dialog was created.");
		}
		this->_msgHandler.on_message(msg, std::move(func));
	}

	template<typename F>
	void on_message(std::initializer_list<UINT> msgs, F&& func)
	{
		if (this->_hWnd) {
			throw std::logic_error("Cannot add message handlers after the dialog was created.");
		}
		this->_msgHandler.on_message(msgs, std::move(func));
	}

	template<typename F>
	void on_command(WORD cmd, F&& func)
	{
		if (this->_hWnd) {
			throw std::logic_error("Cannot add a command handler after the dialog was created.");
		}
		this->_msgHandler.on_command(cmd, std::move(func));
	}

	template<typename F>
	void on_command(std::initializer_list<WORD> cmds, F&& func)
	{
		if (this->_hWnd) {
			throw std::logic_error("Cannot add command handlers after the dialog was created.");
		}
		this->_msgHandler.on_command(cmds, std::move(func));
	}

	template<typename F>
	void on_notify(UINT_PTR idFrom, int code, F&& func)
	{
		if (this->_hWnd) {
			throw std::logic_error("Cannot add a notify handler after the dialog was created.");
		}
		this->_msgHandler.on_notify(idFrom, code, std::move(func));
	}

	template<typename F>
	void on_notify(std::initializer_list<std::pair<UINT_PTR, int>> idFromAndCodes, F&& func)
	{
		if (this->_hWnd) {
			throw std::logic_error("Cannot add notify handlers after the dialog was created.");
		}
		this->_msgHandler.on_notify(idFromAndCodes, std::move(func));
	}

	HWND create_dialog_param(HINSTANCE hInst, const wl::i_window* parent, int dialogId)
	{
		this->_creation_checks(dialogId);

		HWND h = CreateDialogParamW(hInst, MAKEINTRESOURCEW(dialogId),
			parent ? parent->hwnd() : nullptr, _dialog_proc,
			reinterpret_cast<LPARAM>(this)); // pass pointer to self

		if (!h) {
			throw std::system_error(GetLastError(), std::system_category(),
				"CreateDialogParam failed.");
		}
		return h; // our _hWnd member is set during WM_INITDIALOG processing
	}

	INT_PTR dialog_box_param(HINSTANCE hInst, const wl::i_window* parent, int dialogId)
	{
		this->_creation_checks(dialogId);

		// This function blocks until the dialog is closed.
		INT_PTR ret = DialogBoxParamW(hInst, MAKEINTRESOURCEW(dialogId),
			parent ? parent->hwnd() : nullptr, _dialog_proc,
			reinterpret_cast<LPARAM>(this)); // pass pointer to self

		DWORD lerr = GetLastError();
		if (lerr == -1) {
			throw std::system_error(lerr, std::system_category(),
				"DialogBoxParam failed.");
		}
		return ret;
	}

private:
	void _creation_checks(int dialogId)
	{
		if (this->_hWnd) {
			throw std::logic_error("Cannot create a dialog twice.");
		} else if (!dialogId) {
			throw std::logic_error("No dialog resource ID given on dialog setup.");
		}
	}

	static INT_PTR CALLBACK _dialog_proc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp) noexcept
	{
		base_dialog* pSelf = nullptr;

		if (msg == WM_INITDIALOG) {
			pSelf = reinterpret_cast<base_dialog*>(lp);
			SetWindowLongPtrW(hWnd, DWLP_USER, reinterpret_cast<LONG_PTR>(pSelf)); // store
			_set_ui_font_on_children(hWnd);
			pSelf->_hWnd = hWnd; // store HWND in class member
		} else {
			pSelf = reinterpret_cast<base_dialog*>(GetWindowLongPtrW(hWnd, DWLP_USER)); // retrieve
		}

		// If no pointer stored, then no processing is done.
		// Prevents processing before WM_INITDIALOG and after WM_NCDESTROY.
		if (!pSelf) {
			return FALSE;
		}

		// Execute user handler, if any.
		std::optional<LRESULT> ret = pSelf->_msgHandler.exec(msg, wp, lp);

		if (msg == WM_NCDESTROY) {
			SetWindowLongPtrW(hWnd, DWLP_USER, 0); // clear passed pointer
			pSelf->_hWnd = nullptr; // clear stored HWND
		}
		return ret.value_or(FALSE);
	}

	static void _set_ui_font_on_children(HWND hDlg) noexcept
	{
		SendMessageW(hDlg, WM_SETFONT,
			reinterpret_cast<WPARAM>(globalUiFont.hfont()), MAKELPARAM(FALSE, 0));
		EnumChildWindows(hDlg, [](HWND hWnd, LPARAM lp) noexcept -> BOOL {
			SendMessageW(hWnd, WM_SETFONT,
				reinterpret_cast<WPARAM>(reinterpret_cast<HFONT>(lp)),
				MAKELPARAM(FALSE, 0)); // will run on each child
			return TRUE;
		}, reinterpret_cast<LPARAM>(globalUiFont.hfont()));
	}
};

}//namespace _wli