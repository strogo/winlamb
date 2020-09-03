/**
 * Part of WinLamb - Win32 API Lambda Library
 * https://github.com/rodrigocfd/winlamb
 * This library is released under the MIT License
 */

#pragma once
#include <chrono>
#include <string_view>
#include <Windows.h>
#include <CommCtrl.h>
#include "internals/base_native_control.h"
#include "internals/control_visuals.h"
#include "internals/gdi_obj.h"
#include "internals/interfaces.h"
#include "time.h"

namespace wl {

// Native date and time picker control.
// https://docs.microsoft.com/en-us/windows/win32/controls/date-and-time-picker-controls
class date_time_picker : public i_control {
private:
	_wli::base_native_control _base;

public:
	date_time_picker() = default;
	date_time_picker(date_time_picker&&) = default;
	date_time_picker& operator=(date_time_picker&&) = default; // movable only

	// Returns the HWND.
	[[nodiscard]] HWND hwnd() const noexcept override { return this->_base.hwnd(); }

	// Retrieves the control ID.
	[[nodiscard]] int id() const noexcept override { return this->_base.id(); }

	// Calls EnableWindow().
	const date_time_picker& enable(bool isEnabled) const noexcept { EnableWindow(this->hwnd(), isEnabled); return *this; }

	// Assigns this object to a control in a dialog window.
	date_time_picker& assign(const i_window* parent, int ctrlId) { this->_base.assign(parent, ctrlId); return *this; }

	// Installs a window subclass and adds a handle to a message.
	// func: [](wl::param::any p) -> LRESULT { }
	template<typename F>
	void on_subclass_message(UINT msg, F&& func) { this->_base.on_subclass_message(msg, std::move(func)); }

	// Installs a window subclass and adds a handle to a message.
	// func: [](wl::param::any p) -> LRESULT { }
	template<typename F>
	void on_subclass_message(std::initializer_list<UINT> msgs, F&& func) { this->_base.on_subclass_message(msgs, std::move(func)); }

	// Calls CreateWindowEx().
	// Should be called during parent's WM_CREATE processing.
	// Position and size will be adjusted to match current system DPI.
	date_time_picker& create(const i_window* parent, int id,
		POINT pos, SIZE size = {110, 21}, DWORD dtpStyles = 0)
	{
		pos = _wli::multiply_dpi(pos);
		size = _wli::multiply_dpi(size);

		HWND h = this->_base.create_window(parent, id, DATETIMEPICK_CLASSW, {}, pos, size,
			WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_GROUP | dtpStyles, 0);

		_wli::globalUiFont.set_on_control(*this);
		return *this;
	}

	// Returns a std::chrono::time_point value representing the time.
	[[nodiscard]] std::chrono::system_clock::time_point time() const
	{
		SYSTEMTIME st{};
		DateTime_GetSystemtime(this->hwnd(), &st);
		return time::to_time_point(st);
	}

	// Sets the displaying format for the control.
	// https://docs.microsoft.com/en-us/windows/win32/controls/date-and-time-picker-controls#format-strings
	const date_time_picker& set_format(std::wstring_view fmtStr) const noexcept
	{
		DateTime_SetFormat(this->hwnd(), fmtStr.data());
		return *this;
	}

	// Sets the chrono::time_point value representing the time.
	const date_time_picker& set_time(
		std::chrono::system_clock::time_point newTime) const
	{
		SYSTEMTIME st = time::to_systemtime(newTime);
		DateTime_SetSystemtime(this->hwnd(), GDT_VALID, &st);
		return *this;
	}
};

}//namespace wl