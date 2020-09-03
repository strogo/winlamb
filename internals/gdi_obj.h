/**
 * Part of WinLamb - Win32 API Lambda Library
 * https://github.com/rodrigocfd/winlamb
 * This library is released under the MIT License
 */

#pragma once
#include <stdexcept>
#include <string_view>
#include <type_traits>
#include <Windows.h>
#include <VersionHelpers.h>
#include "interfaces.h"
#include "../str.h"

namespace _wli {

// Base to all GDI objects.
// Calls DeleteObject() on destructor.
// Can be instantiated only by derived classes.
template<typename H>
class gdi_obj {
protected:
	H _h = nullptr;

	gdi_obj() = default;
	gdi_obj(H hGdiObj) noexcept : _h{hGdiObj} { }

	gdi_obj& operator=(H hGdiObj) noexcept
	{
		this->delete_object();
		this->_h = hGdiObj;
		return *this;
	}

public:
	virtual ~gdi_obj() { this->delete_object(); }
	gdi_obj(gdi_obj&& other) noexcept { this->operator=(std::move(other)); } // movable only

	gdi_obj& operator=(gdi_obj&& other) noexcept
	{
		this->delete_object();
		std::swap(this->_h, other._h);
		return *this;
	}

	// Returns the underlying handle converted to HGDIOBJ.
	[[nodiscard]] HGDIOBJ hgdiobj() const noexcept { return this->_h; }

	// Calls DeleteObject().
	void delete_object() noexcept
	{
		if (this->_h) {
			DeleteObject(this->_h);
			this->_h = nullptr;
		}
	}
};

}//namespace _wli

namespace wl::gdi {

// Manages an HBITMAP resource.
// Calls DeleteObject() on destructor.
class bitmap final : public _wli::gdi_obj<HBITMAP> {
public:
	bitmap() = default;
	bitmap(HBITMAP hBmp) noexcept : gdi_obj{hBmp} { }
	bitmap(bitmap&& other) noexcept : gdi_obj{std::move(other)} {  }

	bitmap& operator=(HBITMAP hBmp) noexcept { this->gdi_obj::operator=(hBmp); return *this; }
	bitmap& operator=(bitmap&& other) noexcept { this->gdi_obj::operator=(std::move(other)); return *this; }

	// Returns the HBITMAP.
	[[nodiscard]] HBITMAP hbitmap() const noexcept { return this->_h; }
};

// Manages an HBRUSH resource.
// Calls DeleteObject() on destructor.
class brush final : public _wli::gdi_obj<HBRUSH> {
public:
	brush() = default;
	brush(HBRUSH hBrush) noexcept : gdi_obj{hBrush} { }
	brush(brush&& other) noexcept : gdi_obj{std::move(other)} {  }

	brush& operator=(HBRUSH hBrush) noexcept { this->gdi_obj::operator=(hBrush); return *this; }
	brush& operator=(brush&& other) noexcept { this->gdi_obj::operator=(std::move(other)); return *this; }

	// Returns the HBRUSH.
	[[nodiscard]] HBRUSH hbrush() const noexcept { return this->_h; }
};

// Manages an HFONT resource.
// Calls DeleteObject() on destructor.
class font final : public _wli::gdi_obj<HFONT> {
public:
	enum class deco : BYTE {
		NONE      = 0b0000'0000,
		BOLD      = 0b0000'0001,
		ITALIC    = 0b0000'0010,
		STRIKEOUT = 0b0000'0100,
		UNDERLINE = 0b0000'1000
	};

	font() = default;
	font(HFONT hFont) noexcept : gdi_obj{hFont} { }
	font(font&& other) noexcept : gdi_obj{std::move(other)} { }

	font& operator=(HFONT hFont) noexcept { this->gdi_obj::operator=(hFont); return *this; }
	font& operator=(font&& other) noexcept { this->gdi_obj::operator=(std::move(other)); return *this; }

	// Returns the HFONT.
	[[nodiscard]] HFONT hfont() const noexcept { return this->_h; }

	// Creates a font with just a few parameters.
	font& create(std::wstring_view faceName, BYTE size, deco style = deco::NONE)
	{
		this->delete_object();
		LOGFONT lf{};
		lstrcpyW(lf.lfFaceName, faceName.data());
		lf.lfHeight = -(size + 3);

		auto hasDeco = [=](deco yourDeco) noexcept -> BOOL {
			return (static_cast<BYTE>(style) &
				static_cast<BYTE>(yourDeco)) != 0 ? TRUE : FALSE;
		};

		lf.lfWeight    = hasDeco(deco::BOLD) ? FW_BOLD : FW_DONTCARE;
		lf.lfItalic    = hasDeco(deco::ITALIC);
		lf.lfUnderline = hasDeco(deco::UNDERLINE);
		lf.lfStrikeOut = hasDeco(deco::STRIKEOUT);

		return this->create_indirect(lf);
	}

	// Calls CreateFontIndirect().
	font& create_indirect(const LOGFONT& lplf)
	{
		this->delete_object();
		this->_h = CreateFontIndirectW(&lplf);
		if (!this->_h) {
			throw std::runtime_error(
				str::unicode_to_ansi(
					str::format(L"CreateFontIndirect failed for \"%s\".", lplf.lfFaceName) ));
		}
		return *this;
	}

	// Creates the same exact font used by UI, usually Segoe 9.
	font& create_ui()
	{
		this->delete_object();

		NONCLIENTMETRICS ncm{};
		ncm.cbSize = sizeof(ncm);
		if (!IsWindowsVistaOrGreater()) {
			ncm.cbSize -= sizeof(ncm.iBorderWidth);
		}
		SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, ncm.cbSize, &ncm, 0);
		return this->create_indirect(ncm.lfMenuFont);
	}

	// Calls WM_SETFONT on the given control.
	const font& set_on_control(const i_control& child) const noexcept
	{
		SendMessageW(child.hwnd(), WM_SETFONT,
			reinterpret_cast<WPARAM>(_h), TRUE);
		return *this;
	}

	// Calls WM_SETFONT on the given control.
	const font& set_on_control(
		std::initializer_list<std::reference_wrapper<const i_control>> children) const noexcept
	{
		for (const i_control& child : children) {
			this->set_on_control(child);
		}
		return *this;
	}

	// Static method; checks if the font is currently installed.
	// Face names are case-insensitive.
	[[nodiscard]] static bool face_name_exists(std::wstring_view faceName)
	{
		// http://cboard.cprogramming.com/windows-programming/90066-how-determine-if-font-support-unicode.html

		bool isInstalled = false;

		HDC hdc = GetDC(nullptr);
		if (!hdc) {
			throw std::system_error(GetLastError(), std::system_category(),
				str::unicode_to_ansi(
					str::format(L"GetDC failed when checking if \"%s\" exists") ));
		}

		EnumFontFamiliesW(hdc, faceName.data(),
			[](const LOGFONT*, const TEXTMETRIC*, DWORD, LPARAM lp) noexcept -> int
			{
				bool* pIsInstalled = reinterpret_cast<bool*>(lp);
				*pIsInstalled = true; // if we're here, font does exist
				return 0;
			},
			reinterpret_cast<LPARAM>(&isInstalled));

		ReleaseDC(nullptr, hdc);
		return isInstalled;
	}
};

// Manages an HPEN resource.
// Calls DeleteObject() on destructor.
class pen final : public _wli::gdi_obj<HPEN> {
public:
	pen() = default;
	pen(HPEN hPen) noexcept : gdi_obj{hPen} { }
	pen(pen&& other) noexcept : gdi_obj{std::move(other)} {  }

	pen& operator=(HPEN hPen) noexcept { this->gdi_obj::operator=(hPen); return *this; }
	pen& operator=(pen&& other) noexcept { this->gdi_obj::operator=(std::move(other)); return *this; }

	// Returns the HPEN.
	[[nodiscard]] HPEN hpen() const noexcept { return this->_h; }
};

// Manages an HRGN resource.
// Calls DeleteObject() on destructor.
class rgn final : public _wli::gdi_obj<HRGN> {
public:
	rgn() = default;
	rgn(HRGN hRgn) noexcept : gdi_obj{hRgn} { }
	rgn(rgn&& other) noexcept : gdi_obj{std::move(other)} {  }

	rgn& operator=(HRGN hRgn) noexcept { this->gdi_obj::operator=(hRgn); return *this; }
	rgn& operator=(rgn&& other) noexcept { this->gdi_obj::operator=(std::move(other)); return *this; }

	// Returns the HRGN.
	[[nodiscard]] HRGN hrgn() const noexcept { return this->_h; }
};

}//namespace wl::gdi

DEFINE_ENUM_FLAG_OPERATORS(wl::gdi::font::deco)

namespace _wli {

 // Used on all old native controls, which don't automatically have it.
inline wl::gdi::font globalUiFont;

}//namespace _wli