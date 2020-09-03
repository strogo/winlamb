/**
 * Part of WinLamb - Win32 API Lambda Library
 * https://github.com/rodrigocfd/winlamb
 * This library is released under the MIT License
 */

#pragma once
#include <stdexcept>
#include <string_view>
#include <system_error>
#include <Windows.h>
#include <CommCtrl.h>
#include <commoncontrols.h> // IID_IImageList
#include "internals/interfaces.h"
#include "com.h"

namespace wl {

// Manages an HICON resource.
// Calls DestroyIcon() on destructor.
class icon final {
private:
	HICON _hIcon = nullptr;

public:
	enum class size {
		SMALL16      = SHIL_SMALL,
		LARGE32      = SHIL_LARGE,
		EXTRALARGE48 = SHIL_EXTRALARGE,
		JUMBO256     = SHIL_JUMBO
	};

	~icon() { this->destroy(); }
	icon() = default;
	icon(icon&& other) noexcept { this->operator=(std::move(other)); } // movable only

	icon& operator=(icon&& other) noexcept
	{
		this->destroy();
		std::swap(this->_hIcon, other._hIcon);
		return *this;
	}

	// Returns the HICON.
	[[nodiscard]] HICON hicon() const noexcept { return this->_hIcon; }

	// Retrieves the size dynamically with GetObject().
	[[nodiscard]] size calc_size() const
	{
		SIZE sz{}; // http://stackoverflow.com/a/13923853

		if (this->_hIcon) {
			BITMAP bmp{};
			ICONINFO nfo{};
			GetIconInfo(this->_hIcon, &nfo);

			if (nfo.hbmColor) {
				int nWrittenBytes = GetObjectW(nfo.hbmColor, sizeof(bmp), &bmp);
				if (nWrittenBytes > 0) {
					sz.cx = bmp.bmWidth;
					sz.cy = bmp.bmHeight;
					//myinfo.nBitsPerPixel = bmp.bmBitsPixel;
				}
			} else if (nfo.hbmMask) {
				int nWrittenBytes = GetObjectW(nfo.hbmMask, sizeof(bmp), &bmp);
				if (nWrittenBytes > 0) {
					sz.cx = bmp.bmWidth;
					sz.cy = bmp.bmHeight / 2;
					//myinfo.nBitsPerPixel = 1;
				}
			}

			if (nfo.hbmColor) DeleteObject(nfo.hbmColor);
			if (nfo.hbmMask) DeleteObject(nfo.hbmMask);
		}

		if (sz.cx == 16 && sz.cy == 16) {
			return size::SMALL16;
		} else if (sz.cx == 32 && sz.cy == 32) {
			return size::LARGE32;
		} else if (sz.cx == 48 && sz.cy == 48) {
			return size::EXTRALARGE48;
		} else if (sz.cx == 256 && sz.cy == 256) {
			return size::JUMBO256;
		}

		throw std::runtime_error("Unknown icon size.");
	}

	// Calls DestroyIcon().
	icon& destroy() noexcept
	{
		if (this->_hIcon) {
			DestroyIcon(this->_hIcon);
			this->_hIcon = nullptr;
		}
		return *this;
	}

	// Loads an icon into a static control; the icon can be safely destroyed then.
	// On the resource editor, change "Type" property to "Icon".
	const icon& draw_in_label(HWND hStatic) const noexcept
	{
		SendMessageW(hStatic, STM_SETIMAGE, IMAGE_ICON,
			reinterpret_cast<LPARAM>(this->_hIcon));
		return *this;
	}

	// Loads an application icon resource.
	icon& load_app_resource(int iconId, size iconSize)
	{
		return this->_raw_load_resource(iconId, iconSize, GetModuleHandle(nullptr));
	}

	// Loads the icon used by Windows Explorer to represent the given file type.
	icon& load_shell_file_type(std::wstring_view fileExtension, size iconSize)
	{
		this->destroy();

		wchar_t extens[16]{}; // arbitrary length
		lstrcpyW(extens, (fileExtension[0] == L'.') ? L"*" : L"*."); // prepend dot if it doesn't have
		lstrcatW(extens, fileExtension.data());

		com::lib comLib{com::lib::init::NOW};
		SHFILEINFO shfi{};

		if (iconSize == size::SMALL16 || iconSize == size::LARGE32) { // http://stackoverflow.com/a/28015423
			if (!SHGetFileInfoW(extens, FILE_ATTRIBUTE_NORMAL, &shfi, sizeof(shfi),
				SHGFI_USEFILEATTRIBUTES | SHGFI_ICON |
				(iconSize == size::SMALL16 ? SHGFI_SMALLICON : SHGFI_LARGEICON)) )
			{
				throw std::runtime_error(
					"SHGetFileInfo failed when trying to load icon from shell.");
			}
			this->_hIcon = shfi.hIcon;

		} else {
			IImageList* pImgList = nullptr; // http://stackoverflow.com/a/30496252
			HRESULT hr =SHGetImageList(static_cast<int>(iconSize),
					IID_IImageList, reinterpret_cast<void**>(&pImgList));
			if (FAILED(hr)) {
				throw std::system_error(hr, std::system_category(),
					"SHGetImageList failed when trying to load icon from shell");
			}

			if (!SHGetFileInfoW(extens, FILE_ATTRIBUTE_NORMAL, &shfi, sizeof(shfi),
				SHGFI_USEFILEATTRIBUTES | SHGFI_SYSICONINDEX) )
			{
				throw std::runtime_error(
					"SHGetFileInfo failed when trying to load system's image list.");
			}
			this->_hIcon = ImageList_GetIcon(reinterpret_cast<HIMAGELIST>(pImgList),
				shfi.iIcon, ILD_NORMAL);
		}

		return *this;
	}

	// Loads a system icon resource.
	icon& load_sys_resource(int iconId, size iconSize)
	{
		return this->_raw_load_resource(iconId, iconSize, nullptr);
	}

	// Converts the size enum to the corresponding SIZE.
	[[nodiscard]] static SIZE convert_size_to_value(size iconSize) noexcept
	{
		switch (iconSize) {
			case size::JUMBO256:     return {256, 256};
			case size::EXTRALARGE48: return {48, 48};
			case size::SMALL16:      return {16, 16};
		}
		return {32, 32};
	}

private:
	icon& _raw_load_resource(int iconId, size iconSize, HINSTANCE hInst)
	{
		this->destroy();

		SIZE sz = convert_size_to_value(iconSize);
		this->_hIcon = static_cast<HICON>(
			LoadImageW(hInst, MAKEINTRESOURCEW(iconId), IMAGE_ICON,
				static_cast<int>(sz.cx), static_cast<int>(sz.cy),
				LR_DEFAULTCOLOR) );

		if (!this->_hIcon) {
			throw std::system_error(GetLastError(), std::system_category(),
				"LoadImage failed when trying to load icon resource.");
		}
	}
};

}//namespace wl