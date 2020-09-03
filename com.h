/**
 * Part of WinLamb - Win32 API Lambda Library
 * https://github.com/rodrigocfd/winlamb
 * This library is released under the MIT License
 */

#pragma once
#include <system_error>
#include <Windows.h>
#include <objbase.h>
#include "internals/com_bstr.h"
#include "internals/com_ptr.h"
#include "internals/com_variant.h"

// Utilities to COM library and objects.
namespace wl::com {

// Automates CoInitialize() and CoUninitialize() calls with RAII.
class lib final {
private:
	bool _running = false;

public:
	enum class init {
		// CoInitialize() will be called in the constructor, right now.
		NOW,
		// CoInitialize() won't be called right now.
		// You must manually call initialize() later.
		LATER
	};

	~lib() { this->un_initialize(); } // https://stackoverflow.com/q/47123650/6923555
	lib(lib&& other) noexcept { this->operator=(std::move(other)); } // movable only

	explicit lib(init when)
	{
		if (when == init::NOW) { // otherwise you must manually call initialize()
			this->initialize();
		}
	}

	lib& operator=(lib&& other) noexcept
	{
		this->un_initialize();
		std::swap(this->_running, other._running);
		return *this;
	}

	// Calls CoInitialize(), can be carelessly called multiple times.
	void initialize()
	{
		if (!this->_running) {
			HRESULT hr = CoInitialize(nullptr);
			if (hr != S_OK && hr != S_FALSE) {
				throw std::system_error(GetLastError(), std::system_category(),
					"CoInitialize failed.");
			}
			this->_running = true;
		}
	}

	// Calls CoUninitialize(), can be carelessly called multiple times.
	void un_initialize() noexcept
	{
		if (this->_running) {
			CoUninitialize();
			this->_running = false;
		}
	}
};

}//namespace wl::com