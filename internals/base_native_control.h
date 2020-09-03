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
#include <CommCtrl.h>
#include "catch_all_excps.h"
#include "interfaces.h"
#include "store.h"

namespace _wli {

// Owns the HWND.
// Owns the subclass message handler store.
// Provides the subclass procedure.
class base_native_control final {
private:
	HWND _hWnd = nullptr;
	UINT_PTR _subclassId = 0;
	store<UINT> _msgs; // user lambdas for subclassing

public:
	[[nodiscard]] HWND hwnd() const noexcept { return this->_hWnd; }
	[[nodiscard]] int  id() const noexcept   { return GetDlgCtrlID(this->hwnd()); }

	void assign(const wl::i_window* parent, int ctrlId)
	{
		if (this->_hWnd) {
			throw std::logic_error("Cannot assign an alredy created control.");
		} else if (!parent) {
			throw std::invalid_argument("No parent passed to base_native_control::show().");
		}
		this->_hWnd = GetDlgItem(parent->hwnd(), ctrlId);

		this->_install_subclass_if_needed();
	}

	template<typename F>
	void on_subclass_message(UINT msg, F&& func)
	{
		if (this->_hWnd) {
			throw std::logic_error("Cannot add a subclass handler after the window was created.");
		}
		this->_msgs.add(msg, std::move(func));
	}

	template<typename F>
	void on_subclass_message(std::initializer_list<UINT> msgs, F&& func)
	{
		if (this->_hWnd) {
			throw std::logic_error("Cannot add subclass handlers after the window was created.");
		}
		this->_msgs.add(msgs, std::move(func));
	}

	HWND create_window(const wl::i_window* parent, int id,
		std::wstring_view className, std::optional<std::wstring_view> title,
		POINT pos, SIZE size, DWORD styles, DWORD exStyles)
	{
		if (this->_hWnd) {
			throw std::logic_error("Cannot create a control twice.");
		} else if (!parent) {
			throw std::invalid_argument("No parent passed to base_native_control::show().");
		}

		HWND h = CreateWindowExW(exStyles, className.data(),
			title.has_value() ? title.value().data() : nullptr,
			styles, pos.x, pos.y, size.cx, size.cy, parent->hwnd(),
			reinterpret_cast<HMENU>(static_cast<UINT_PTR>(id)),
			reinterpret_cast<HINSTANCE>(GetWindowLongPtrW(parent->hwnd(), GWLP_HINSTANCE)),
			nullptr);

		if (!h) {
			throw std::system_error(GetLastError(), std::system_category(),
				"CreateWindowEx failed for native control.");
		}
		this->_hWnd = h; // store HWND in class member

		this->_install_subclass_if_needed();
		return h;
	}

private:
	void _install_subclass_if_needed() noexcept
	{
		if (!this->_msgs.empty()) { // at least 1 subclass message handler was added?
			static UINT_PTR firstId = 0;
			this->_subclassId = ++firstId;
			SetWindowSubclass(this->_hWnd, _subclass_proc, this->_subclassId,
				reinterpret_cast<DWORD_PTR>(this)); // pass pointer to self
		}
	}

	static LRESULT CALLBACK _subclass_proc(HWND hWnd, UINT msg,
		WPARAM wp, LPARAM lp, UINT_PTR idSubclass, DWORD_PTR refData) noexcept
	{
		base_native_control* pSelf = reinterpret_cast<base_native_control*>(refData);
		std::function<LRESULT(wl::param::any)>* userFunc = nullptr;
		LRESULT retVal = 0;

		if (pSelf && pSelf->_hWnd) {
			userFunc = pSelf->_msgs.find(msg); // search an user lambda for this message
			if (userFunc) {
				catch_all_excps([&]() {
					retVal = (*userFunc)({wp, lp}); // execute user lambda
				}, post_quit_on_catch::YES);
			}
		}

		if (msg == WM_NCDESTROY) { // always check
			RemoveWindowSubclass(hWnd, _subclass_proc, idSubclass);
		}
		return userFunc ? retVal : DefSubclassProc(hWnd, msg, wp, lp);
	}
};

}//namespace _wli