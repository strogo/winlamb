/**
 * Part of WinLamb - Win32 API Lambda Library
 * https://github.com/rodrigocfd/winlamb
 * This library is released under the MIT License
 */

#pragma once
#include <system_error>
#include <vector>
#include <Windows.h>

namespace _wli {

const UINT WM_MODELESS_CREATED   = WM_APP + 0x3FFD; // sent by modeless windows
const UINT WM_MODELESS_DESTROYED = WM_APP + 0x3FFE;

// Runs the main application loop.
// Keeps modeless children handles.
class base_main_loop final {
private:
	std::vector<HWND> _hModelessChildren;

public:
	void add_modeless_child(HWND hModeless)
	{
		this->_hModelessChildren.emplace_back(hModeless);
	}

	void delete_modeless_child(HWND hModeless)
	{
		for (auto it = this->_hModelessChildren.begin();
			it != this->_hModelessChildren.end();
			++it)
		{
			if (*it == hModeless) {
				this->_hModelessChildren.erase(it);
				break;
			}
		}
	}

	const std::vector<HWND>& modeless_children() noexcept
	{
		return this->_hModelessChildren;
	}

	int run_loop(HWND hWnd, HACCEL hAccel = nullptr) const
	{
		for (;;) {
			MSG msg{};
			BOOL gmRet = GetMessageW(&msg, nullptr, 0, 0);
			if (gmRet == -1) {
				throw std::system_error(GetLastError(), std::system_category(),
					"GetMessage failed.");
			} else if (!gmRet) {
				// WM_QUIT was sent, gracefully terminate the program.
				// wParam has the program exit code.
				// https://docs.microsoft.com/en-us/windows/win32/winmsg/using-messages-and-message-queues
				return static_cast<int>(msg.wParam);
			}

			// Does this message belong to a modeless child window (if any)?
			// http://www.winprog.org/tutorial/modeless_dialogs.html
			if (this->_is_modeless_msg(hWnd, msg)) {
				continue;
			}

			// If a child window, will retrieve its top-level parent.
			// If a top-level, use itself.
			HWND hTopLevel = GetAncestor(msg.hwnd, GA_ROOT);

			// If we have an accelerator table, try to translate the message.
			if (hAccel && TranslateAcceleratorW(hTopLevel, hAccel, &msg)) {
				continue; // message translated
			}

			if (IsDialogMessageW(hTopLevel, &msg)) {
				// Processed all keyboard actions for child controls.
				continue;
			}

			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		}
	}

private:
	bool _is_modeless_msg(HWND hWnd, MSG& msg) const noexcept
	{
		for (HWND hModeless : this->_hModelessChildren) {
			if (!hModeless || !IsChild(hWnd, hModeless)) {
				continue; // skip invalid HWND
			}
			if (IsDialogMessageW(hModeless, &msg)) {
				return true; // it was a message for this modeless, it was processed and we're done
			}
		}
		return false; // message wasn't for any of the modeless children
	}
};

}//namespace _wli