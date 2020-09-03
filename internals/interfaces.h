/**
 * Part of WinLamb - Win32 API Lambda Library
 * https://github.com/rodrigocfd/winlamb
 * This library is released under the MIT License
 */

#pragma once
#include <Windows.h>

namespace wl {

// Pure abstract class, base to all windows.
class i_window {
public:
	virtual ~i_window() { }

	// Returns the HWND.
	[[nodiscard]] virtual HWND hwnd() const noexcept = 0;
};

// Pure abstract class, base to all controls.
class i_control : public i_window {
public:
	virtual ~i_control() { }

	// Retrieves the control ID.
	[[nodiscard]] virtual int id() const noexcept = 0;
};

}//namespace wl