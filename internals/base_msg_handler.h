/**
 * Part of WinLamb - Win32 API Lambda Library
 * https://github.com/rodrigocfd/winlamb
 * This library is released under the MIT License
 */

#pragma once
#include <optional>
#include <process.h>
#include <Windows.h>
#include "catch_all_excps.h"
#include "param.h"
#include "store.h"

namespace _wli {

// Owns the message handler store.
// Handles the UI thread message.
class base_msg_handler final {
public:
	struct thread_pack final {
		std::function<void()> func;
	};

private:
	static const UINT WM_UI_THREAD = WM_APP + 0x3FFF; // used only within this class

	store<UINT> _msgs; // user lambdas for messages, WM_COMMAND and WM_NOTIFY
	store<WORD> _cmds;
	store<std::pair<UINT_PTR, int>> _nfys; // NMHDR is UINT_PTR/UINT, but UINT notif codes are mostly negative, triggering overflow warnings

public:
	base_msg_handler()
	{
		this->_default_msg_handlers();
	}

	template<typename F>
	void on_message(UINT msg, F&& func) { this->_msgs.add(msg, std::move(func)); }

	template<typename F>
	void on_message(std::initializer_list<UINT> msgs, F&& func) { this->_msgs.add(msgs, std::move(func)); }

	template<typename F>
	void on_command(WORD cmd, F&& func) { this->_cmds.add(cmd, std::move(func)); }

	template<typename F>
	void on_command(std::initializer_list<WORD> cmds, F&& func) { this->_cmds.add(cmds, std::move(func)); }

	template<typename F>
	void on_notify(UINT_PTR idFrom, int code, F&& func) { this->_nfys.add({idFrom, code}, std::move(func)); }

	template<typename F>
	void on_notify(std::initializer_list<std::pair<UINT_PTR, int>> idFromAndCodes, F&& func) { this->_nfys.add(idFromAndCodes, std::move(func)); }

	// Searches for a stored handler for the given message and executes it, if any.
	std::optional<LRESULT> exec(UINT msg, WPARAM wp, LPARAM lp) noexcept
	{
		// Search a stored user handler.
		std::function<LRESULT(wl::param::any)>* userFunc = nullptr;

		if (msg == WM_COMMAND) {
			userFunc = this->_cmds.find(LOWORD(wp)); // search an user lambda for this WM_COMMAND
		} else if (msg == WM_NOTIFY) {
			const NMHDR* nmhdr = reinterpret_cast<const NMHDR*>(lp);
			userFunc = this->_nfys.find({nmhdr->idFrom, static_cast<int>(nmhdr->code)}); // search an user lambda for this WM_NOTIFY
		} else {
			userFunc = this->_msgs.find(msg); // search an user lambda for this message
		}

		if (userFunc) {
			LRESULT retVal = 0;
			catch_all_excps([&]() {
				retVal = (*userFunc)({wp, lp}); // execute user lambda
			}, post_quit_on_catch::YES);
			return retVal;
		}

		return {}; // message handler not found
	}

	// This method is analog to SendMessage (synchronous), but intended to be called
	// from another thread, so a callback function can, tunelled by wndproc, run in
	// the original thread of the window, thus allowing GUI updates. This avoids the
	// user to deal with a custom WM_ message.
	// func: []() { }
	template<typename F>
	void run_thread_ui(HWND hWnd, F&& func)
	{
		thread_pack* pPack = new thread_pack{std::move(func)};
		SendMessageW(hWnd, WM_UI_THREAD, 0xc0def00d, reinterpret_cast<LPARAM>(pPack));
	}

private:
	void _default_msg_handlers()
	{
		this->on_message(WM_UI_THREAD, [](wl::param::any p) noexcept -> LRESULT
		{
			// Handles our special message, sent by run_thread_ui().
			if (p.wparam == 0xc0def00d && p.lparam) {
				thread_pack* pPack = reinterpret_cast<thread_pack*>(p.lparam); // retrieve back
				catch_all_excps([&]() {
					pPack->func(); // invoke user func
				}, post_quit_on_catch::YES);
				delete pPack;
			}
			return 0;
		});
	}
};

}//namespace _wli

namespace wl {

// Executes a function asynchronously, in a new detached thread.
// Analog to std::thread([](){ ... }).detach(), but lighter and exception-safe.
// func: []() { }
template<typename F>
inline void run_thread_detached(F&& func) noexcept
{
	auto pPack = new _wli::base_msg_handler::thread_pack{std::move(func)};

	uintptr_t hThread = _beginthreadex(nullptr, 0,
		[](void* ptr) noexcept -> unsigned int {
			auto pPack = reinterpret_cast<_wli::base_msg_handler::thread_pack*>(ptr);
			_wli::catch_all_excps([&]() {
				pPack->func(); // invoke user func
			}, _wli::post_quit_on_catch::NO);
			delete pPack;
			_endthreadex(0); // http://www.codeproject.com/Articles/7732/A-class-to-synchronise-thread-completions/
			return 0;
		},
		pPack, 0, nullptr);

	CloseHandle(reinterpret_cast<HANDLE>(hThread));
}

}//namespace wl