/**
 * Part of WinLamb - Win32 API Lambda Library
 * https://github.com/rodrigocfd/winlamb
 * This library is released under the MIT License
 */

#pragma once
#include <functional>
#include <optional>
#include <stdexcept>
#include <system_error>
#include <Windows.h>
#include "base_msg_handler.h"
#include "interfaces.h"

namespace _wli {

// Owns the HWND.
// Calls RegisterClassEx() and CreateWindowEx().
// Provides the window procedure.
class base_window final {
private:
	HWND _hWnd = nullptr;
	base_msg_handler _msgHandler;

public:
	~base_window()
	{
		if (this->_hWnd) {
			SetWindowLongPtrW(this->_hWnd, GWLP_USERDATA, 0); // clear passed pointer
		}
	}

	[[nodiscard]] const HWND hwnd() const noexcept { return this->_hWnd; }

	template<typename F>
	void run_thread_ui(F&& func) { this->_msgHandler.run_thread_ui(this->_hWnd, std::move(func)); }

	// Calls RegisterClassEx().
	ATOM register_class(WNDCLASSEXW& wcx) const
	{
		wcx.cbSize = sizeof(WNDCLASSEXW);
		wcx.lpfnWndProc = _window_proc;

		SetLastError(ERROR_SUCCESS);
		ATOM atom = RegisterClassExW(&wcx);
		DWORD lerr = GetLastError();

		if (lerr == ERROR_CLASS_ALREADY_EXISTS) {
			// https://devblogs.microsoft.com/oldnewthing/20150429-00/?p=44984
			// https://devblogs.microsoft.com/oldnewthing/20041011-00/?p=37603
			atom = GetClassInfoExW(wcx.hInstance, wcx.lpszClassName, &wcx);
		} else if (lerr != ERROR_SUCCESS) {
			throw std::system_error(lerr, std::system_category(),
				"RegisterClassEx failed.");
		}
		return atom;
	}

	// Calls CreateWindowEx().
	// Coordinates won't be adjusted to system DPI.
	HWND create_window(HINSTANCE hInst, const wl::i_window* parent,
		std::wstring_view className, std::optional<std::wstring_view> title,
		HMENU hMenu, POINT pos, SIZE size, DWORD exStyles, DWORD styles)
	{
		if (this->_hWnd) {
			throw std::logic_error("Cannot create a window twice.");
		}

		HWND h = CreateWindowExW(exStyles, className.data(),
			title.has_value() ? title.value().data() : nullptr,
			styles, pos.x, pos.y, size.cx, size.cy,
			parent ? parent->hwnd() : nullptr, hMenu, hInst,
			reinterpret_cast<LPVOID>(this)); // pass pointer to self

		if (!h) {
			throw std::system_error(GetLastError(), std::system_category(),
				"CreateWindowEx failed.");
		}
		return h; // our _hWnd member is set during WM_NCCREATE processing
	}

	template<typename F>
	void on_message(UINT msg, F&& func)
	{
		if (this->_hWnd) {
			throw std::logic_error("Cannot add a message handler after the window was created.");
		}
		this->_msgHandler.on_message(msg, std::move(func));
	}

	template<typename F>
	void on_message(std::initializer_list<UINT> msgs, F&& func)
	{
		if (this->_hWnd) {
			throw std::logic_error("Cannot add message handlers after the window was created.");
		}
		this->_msgHandler.on_message(msgs, std::move(func));
	}

	template<typename F>
	void on_command(WORD cmd, F&& func)
	{
		if (this->_hWnd) {
			throw std::logic_error("Cannot add a command handler after the window was created.");
		}
		this->_msgHandler.on_command(cmd, std::move(func));
	}

	template<typename F>
	void on_command(std::initializer_list<WORD> cmds, F&& func)
	{
		if (this->_hWnd) {
			throw std::logic_error("Cannot add command handlers after the window was created.");
		}
		this->_msgHandler.on_command(cmds, std::move(func));
	}

	template<typename F>
	void on_notify(UINT_PTR idFrom, int code, F&& func)
	{
		if (this->_hWnd) {
			throw std::logic_error("Cannot add a notify handler after the window was created.");
		}
		this->_msgHandler.on_notify(idFrom, code, std::move(func));
	}

	template<typename F>
	void on_notify(std::initializer_list<std::pair<UINT_PTR, int>> idFromAndCodes, F&& func)
	{
		if (this->_hWnd) {
			throw std::logic_error("Cannot add notify handlers after the window was created.");
		}
		this->_msgHandler.on_notify(idFromAndCodes, std::move(func));
	}

	// If cursor is not set, pick the default arrow.
	static void wcx_set_cursor(HCURSOR setupCursor, WNDCLASSEXW& wcx)
	{
		wcx.hCursor = setupCursor
			? setupCursor
			: LoadCursorW(nullptr, IDC_ARROW);
	}

	// Generates a string hash that uniquely identifies a WNDCLASSEX object.
	// This hash can be used as the class name.
	// Must be called after all WNDCLASSEX fields are set, so they can be hashed.
	[[nodiscard]] static std::wstring wcx_generate_hash(const WNDCLASSEXW& wcx)
	{
		return wl::str::format(L"WNDCLASS.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x",
			wcx.style, wcx.lpfnWndProc, wcx.cbClsExtra, wcx.cbWndExtra,
			wcx.hInstance, wcx.hIcon, wcx.hCursor, wcx.hbrBackground,
			wcx.lpszMenuName, wcx.hIconSm);
	}

private:
	static LRESULT CALLBACK _window_proc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp) noexcept
	{
		base_window* pSelf = nullptr;

		if (msg == WM_NCCREATE) {
			const CREATESTRUCTW* pCs = reinterpret_cast<const CREATESTRUCTW*>(lp);
			pSelf = reinterpret_cast<base_window*>(pCs->lpCreateParams);
			SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pSelf)); // store
			pSelf->_hWnd = hWnd; // store HWND in class member
		} else {
			pSelf = reinterpret_cast<base_window*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA)); // retrieve
		}

		// If no pointer stored, then no processing is done.
		// Prevents processing before WM_NCCREATE and after WM_NCDESTROY.
		if (!pSelf) {
			return DefWindowProcW(hWnd, msg, wp, lp);
		}

		// Execute user handler, if any.
		std::optional<LRESULT> ret = pSelf->_msgHandler.exec(msg, wp, lp);

		if (msg == WM_NCDESTROY) {
			SetWindowLongPtrW(hWnd, GWLP_USERDATA, 0); // clear passed pointer
			pSelf->_hWnd = nullptr; // clear stored HWND
		}
		return ret.has_value() ? ret.value() : DefWindowProcW(hWnd, msg, wp, lp);
	}
};

}//namespace _wli