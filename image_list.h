/**
 * Part of WinLamb - Win32 API Lambda Library
 * https://github.com/rodrigocfd/winlamb
 * This library is released under the MIT License
 */

#pragma once
#include <stdexcept>
#include <Windows.h>
#include <CommCtrl.h>
#include "icon.h"

namespace wl {

// Manages a native image list.
// Calls ImageList_Destroy() on destructor.
// https://docs.microsoft.com/en-us/windows/win32/controls/image-lists
class image_list final {
private:
	HIMAGELIST _hImL = nullptr;
	icon::size _iconSize = icon::size::SMALL16;

public:
	~image_list()
	{
		// A list view will automatically destroy its associated image list,
		// unless it's created with LVS_SHAREIMAGELISTS.
		// System image lists should not be destroyed.
		// http://www.catch22.net/tuts/win32/system-image-list
		// http://www.autohotkey.com/docs/commands/ListView.htm
		this->destroy();
	}

	// The ilcFlags defaults to ILC_COLOR32.
	image_list(icon::size iconSize,
		WORD capacity = 1, WORD capacityGrowRate = 1, UINT ilcFlags = ILC_COLOR32)
	{
		if (this->_hImL) {
			throw std::logic_error("Cannot create image list twice.");
		}

		this->_iconSize = iconSize;
		SIZE sz = icon::convert_size_to_value(iconSize);

		this->_hImL = ImageList_Create(sz.cx, sz.cy, ilcFlags,
			static_cast<int>(capacity), static_cast<int>(capacityGrowRate));

		if (!this->_hImL) {
			throw std::runtime_error("ImageList_Create failed");
		}
	}

	image_list(image_list&& other) noexcept { this->operator=(std::move(other)); }  // movable only

	image_list& operator=(image_list&& other) noexcept
	{
		this->destroy();
		std::swap(this->_hImL, other._hImL);
		std::swap(this->_iconSize, other._iconSize);
		return *this;
	}

	// Returns the HIMAGELIST.
	[[nodiscard]] HIMAGELIST himagelist() const noexcept { return this->_hImL; }

	// Returns the size of the icons within this image list.
	[[nodiscard]] icon::size icon_size() const noexcept { return this->_iconSize; }

	// Calls ImageList_Destroy().
	image_list& destroy() noexcept
	{
		if (this->_hImL) {
			ImageList_Destroy(this->_hImL);
			this->_hImL = nullptr;
			this->_iconSize = icon::size::SMALL16;
		}
		return *this;
	}

	// Makes a clone of the icon and adds it.
	const image_list& clone_icon_and_add(HICON hIcon) const noexcept
	{
		// The icon can be destroyed right away.
		ImageList_AddIcon(this->_hImL, hIcon);
		return *this;
	}

	// Makes a clone of the icon and adds it.
	const image_list& clone_icon_and_add(const icon& ico) const noexcept
	{
		return this->clone_icon_and_add(ico.hicon());
	}

	// Loads an application icon resource.
	const image_list& load_from_resource(int iconId, HINSTANCE hInst = nullptr) const
	{
		// Icons loaded from resource files don't need to be destroyed.
		icon tmpIco;
		tmpIco.load_app_resource(iconId, this->_iconSize);
		return this->clone_icon_and_add(tmpIco);
	}

	// Loads the icon used by Windows Explorer to represent the given file type.
	// Ex.: L"mp3".
	const image_list& load_shell_file_type(std::wstring_view fileExtension) const
	{
		icon tmpIco;
		tmpIco.load_shell_file_type(fileExtension, this->_iconSize);
		return this->clone_icon_and_add(tmpIco);
	}
};

}//namespace wl