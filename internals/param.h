/**
 * Part of WinLamb - Win32 API Lambda Library
 * https://github.com/rodrigocfd/winlamb
 * This library is released under the MIT License
 */

#pragma once
#include <algorithm>
#include <string>
#include <vector>
#include <Windows.h>
#include "../menu.h"

#ifndef GET_X_LPARAM
#define GET_X_LPARAM(lp) ((int)(short)LOWORD(lp)) // copied from windowsx.h
#define GET_Y_LPARAM(lp) ((int)(short)HIWORD(lp))
#endif

// Message crackers for messages and notifications.
namespace wl::param {

// Message parameters to any window message, raw WPARAM and LPARAM values.
struct any {
	WPARAM wparam;
	LPARAM lparam;
};

}//namespace wl::param

// Message crackers for ordinary window messages.
namespace wl::param::wm {

#define WINLAMB_EMPTY_WM(structN) \
	struct structN : public any { \
		structN(const any& p) noexcept : any{p} { } \
	};

#define WINLAMB_INHERIT_WM(structN, baseN) \
	struct structN : public baseN { \
		structN(const any& p) noexcept : baseN(p) { } \
	};

struct activate : public any {
	activate(const any& p) noexcept : any{p} { }
	[[nodiscard]] bool is_being_activated() const noexcept              { return LOWORD(this->wparam) != WA_INACTIVE; }
	[[nodiscard]] bool is_activated_not_by_mouse_click() const noexcept { return LOWORD(this->wparam) == WA_ACTIVE; }
	[[nodiscard]] bool is_activated_by_mouse_click() const noexcept     { return LOWORD(this->wparam) == WA_CLICKACTIVE; }
	[[nodiscard]] bool is_minimized() const noexcept                    { return HIWORD(this->wparam) != 0; }
	[[nodiscard]] HWND swapped_window() const noexcept                  { return reinterpret_cast<HWND>(this->lparam); }
};

struct activateapp : public any {
	activateapp(const any& p) noexcept : any{p} { }
	[[nodiscard]] bool  is_being_activated() const noexcept { return this->wparam != FALSE; }
	[[nodiscard]] DWORD thread_id() const noexcept          { return static_cast<DWORD>(this->lparam); }
};

struct askcbformatname : public any {
	askcbformatname(const any& p) noexcept : any{p} { }
	[[nodiscard]] UINT     buffer_sz() const noexcept { return static_cast<UINT>(this->wparam); }
	[[nodiscard]] wchar_t* buffer() const noexcept    { return reinterpret_cast<wchar_t*>(this->lparam); }
};

WINLAMB_EMPTY_WM(cancelmode);

struct capturechanged : public any {
	capturechanged(const any& p) noexcept : any{p} { }
	[[nodiscard]] HWND window_gaining_mouse() const noexcept { return reinterpret_cast<HWND>(this->lparam); }
};

struct changecbchain : public any {
	changecbchain(const any& p) noexcept : any{p} { }
	[[nodiscard]] HWND window_being_removed() const noexcept { return reinterpret_cast<HWND>(this->wparam); }
	[[nodiscard]] HWND next_window() const noexcept          { return reinterpret_cast<HWND>(this->lparam); }
	[[nodiscard]] bool is_last_window() const noexcept       { return this->next_window() == nullptr; }
};

struct char_ : public any {
	char_(const any& p) noexcept : any{p} { }
	[[nodiscard]] WORD char_code() const noexcept           { return static_cast<WORD>(this->wparam); }
	[[nodiscard]] WORD repeat_count() const noexcept        { return LOWORD(this->lparam); }
	[[nodiscard]] BYTE scan_code() const noexcept           { return LOBYTE(HIWORD(this->lparam)); }
	[[nodiscard]] bool is_extended_key() const noexcept     { return (this->lparam >> 24) & 1; }
	[[nodiscard]] bool has_alt_key() const noexcept         { return (this->lparam >> 29) & 1; }
	[[nodiscard]] bool key_previously_down() const noexcept { return (this->lparam >> 30) & 1; }
	[[nodiscard]] bool key_being_released() const noexcept  { return (this->lparam >> 31) & 1; }
};

struct chartoitem : public any {
	chartoitem(const any& p) noexcept : any{p} { }
	[[nodiscard]] WORD char_code() const noexcept         { return LOWORD(this->wparam); }
	[[nodiscard]] WORD current_caret_pos() const noexcept { return HIWORD(this->wparam); }
	[[nodiscard]] HWND hlistbox() const noexcept          { return reinterpret_cast<HWND>(this->lparam); }
};

WINLAMB_EMPTY_WM(childactivate);

WINLAMB_EMPTY_WM(close);

struct command : public any {
	command(const any& p)  noexcept : any{p} { }
	[[nodiscard]] bool is_from_menu() const noexcept        { return HIWORD(this->wparam) == 0; }
	[[nodiscard]] bool is_from_accelerator() const noexcept { return HIWORD(this->wparam) == 1; }
	[[nodiscard]] bool is_from_control() const noexcept     { return !this->is_from_menu() && !this->is_from_accelerator(); }
	[[nodiscard]] WORD menu_id() const noexcept             { return this->control_id(); }
	[[nodiscard]] WORD accelerator_id() const noexcept      { return this->control_id(); }
	[[nodiscard]] WORD control_id() const noexcept          { return LOWORD(this->wparam); }
	[[nodiscard]] WORD control_notif_code() const noexcept  { return HIWORD(this->wparam); }
	[[nodiscard]] HWND control_hwnd() const noexcept        { return reinterpret_cast<HWND>(this->lparam); }
};

struct compacting : public any {
	compacting(const any& p) noexcept : any{p} { }
	[[nodiscard]] UINT cpu_time_ratio() const noexcept { return static_cast<UINT>(this->wparam); }
};

struct compareitem : public any {
	compareitem(const any& p) noexcept : any{p} { }
	[[nodiscard]] WORD                     control_id() const noexcept        { return static_cast<WORD>(this->wparam); }
	[[nodiscard]] const COMPAREITEMSTRUCT& compareitemstruct() const noexcept { return *reinterpret_cast<const COMPAREITEMSTRUCT*>(this->lparam); }
};

struct contextmenu : public any {
	contextmenu(const any& p) noexcept : any{p} { }
	[[nodiscard]] HWND  target() const noexcept { return reinterpret_cast<HWND>(this->wparam); }
	[[nodiscard]] POINT pos() const noexcept    { return {GET_X_LPARAM(this->lparam), GET_Y_LPARAM(this->lparam)}; }
};

struct copydata : public any {
	copydata(const any& p) noexcept : any{p} { }
	[[nodiscard]] const COPYDATASTRUCT& copydatastruct() const noexcept { return *reinterpret_cast<const COPYDATASTRUCT*>(this->lparam); }
};

struct create : public any {
	create(const any& p) noexcept : any{p} { }
	[[nodiscard]] const CREATESTRUCTW& createstruct() const noexcept { return *reinterpret_cast<const CREATESTRUCTW*>(this->lparam); }
private:
	void foo() { }
};

struct ctlcolorbtn : public any {
	ctlcolorbtn(const any& p) noexcept : any{p} { }
	[[nodiscard]] HDC  hdc() const noexcept  { return reinterpret_cast<HDC>(this->wparam); }
	[[nodiscard]] HWND hctl() const noexcept { return reinterpret_cast<HWND>(this->lparam); }
};

WINLAMB_INHERIT_WM(ctlcolordlg, ctlcolorbtn);

WINLAMB_INHERIT_WM(ctlcoloredit, ctlcolorbtn);

WINLAMB_INHERIT_WM(ctlcolorlistbox, ctlcolorbtn);

WINLAMB_INHERIT_WM(ctlcolorscrollbar, ctlcolorbtn);

WINLAMB_INHERIT_WM(ctlcolorstatic, ctlcolorbtn);

WINLAMB_INHERIT_WM(deadchar, char_);

struct deleteitem : public any {
	deleteitem(const any& p) noexcept : any{p} { }
	[[nodiscard]] WORD                    control_id() const noexcept       { return static_cast<WORD>(this->wparam); }
	[[nodiscard]] const DELETEITEMSTRUCT& deleteitemstruct() const noexcept { return *reinterpret_cast<const DELETEITEMSTRUCT*>(this->lparam); }
};

WINLAMB_EMPTY_WM(destroy);

WINLAMB_EMPTY_WM(destroyclipboard);

struct devmodechange : public any {
	devmodechange(const any& p) noexcept : any{p} { }
	[[nodiscard]] const wchar_t* device_name() const noexcept { return reinterpret_cast<const wchar_t*>(this->lparam); }
};

#ifdef _DBT_H // Ras.h
struct devicechange : public wplp {
	devicechange(const wplp& p) : wplp(p) { }
	[[nodiscard]] UINT                                 event() const noexcept                         { return static_cast<UINT>(this->wparam); }
	[[nodiscard]] const DEV_BROADCAST_HDR&             dev_broadcast_hdr() const noexcept             { return *reinterpret_cast<const DEV_BROADCAST_HDR*>(this->lparam); }
	[[nodiscard]] const DEV_BROADCAST_DEVICEINTERFACE& dev_broadcast_deviceinterface() const noexcept { return *reinterpret_cast<const DEV_BROADCAST_DEVICEINTERFACE*>(this->lparam); }
	[[nodiscard]] const DEV_BROADCAST_HANDLE&          dev_broadcast_handle() const noexcept          { return *reinterpret_cast<const DEV_BROADCAST_HANDLE*>(this->lparam); }
	[[nodiscard]] const DEV_BROADCAST_OEM&             dev_broadcast_oem() const noexcept             { return *reinterpret_cast<const DEV_BROADCAST_OEM*>(this->lparam); }
	[[nodiscard]] const DEV_BROADCAST_PORT&            dev_broadcast_port() const noexcept            { return *reinterpret_cast<const DEV_BROADCAST_PORT*>(this->lparam); }
	[[nodiscard]] const DEV_BROADCAST_VOLUME&          dev_broadcast_volume() const noexcept          { return *reinterpret_cast<const DEV_BROADCAST_VOLUME*>(this->lparam); }
};
#endif

struct displaychange : public any {
	displaychange(const any& p) noexcept : any{p} { }
	[[nodiscard]] UINT bits_per_pixel() const noexcept { return static_cast<UINT>(this->wparam); }
	[[nodiscard]] SIZE sz() const noexcept             { return {LOWORD(this->lparam), HIWORD(this->lparam)}; }
};

WINLAMB_EMPTY_WM(drawclipboard);

struct drawitem : public any {
	drawitem(const any& p) noexcept : any{p} { }
	[[nodiscard]] WORD                  control_id() const noexcept     { return static_cast<WORD>(this->wparam); }
	[[nodiscard]] bool                  is_from_menu() const noexcept   { return this->control_id() == 0; }
	[[nodiscard]] const DRAWITEMSTRUCT& drawitemstruct() const noexcept { return *reinterpret_cast<const DRAWITEMSTRUCT*>(this->lparam); }
};

struct dropfiles : public any {
	dropfiles(const any& p) noexcept : any{p} { }
	[[nodiscard]] HDROP hdrop() const noexcept { return reinterpret_cast<HDROP>(this->wparam); }
	[[nodiscard]] UINT  count() const noexcept { return DragQueryFileW(this->hdrop(), 0xFFFF'FFFF, nullptr, 0); }

	// Returns all the dropped the files and calls DragFinish().
	[[nodiscard]] std::vector<std::wstring> files() const
	{
		std::vector<std::wstring> files(this->count()); // return vector, sized
		for (size_t i = 0; i < files.size(); ++i) {
			files[i].resize( // alloc path string
				static_cast<size_t>(DragQueryFileW(this->hdrop(), static_cast<UINT>(i), nullptr, 0)) + 1,
				L'\0');
			DragQueryFileW(this->hdrop(), static_cast<UINT>(i), &files[i][0],
				static_cast<UINT>(files[i].length()));
			files[i].resize(files[i].length() - 1); // trim null
		}
		DragFinish(this->hdrop());
		std::sort(files.begin(), files.end());
		return files;
	}

	[[nodiscard]] POINT pos() const noexcept
	{
		POINT pt{};
		DragQueryPoint(this->hdrop(), &pt);
		return pt;
	}
};

struct enable : public any {
	enable(const any& p) noexcept : any{p} { }
	[[nodiscard]] bool has_been_enabled() const noexcept { return this->wparam != FALSE; }
};

struct endsession : public any {
	endsession(const any& p) noexcept : any{p} { }
	[[nodiscard]] bool is_session_being_ended() const noexcept { return this->wparam != FALSE; }
	[[nodiscard]] bool is_system_issue() const noexcept        { return (this->lparam & ENDSESSION_CLOSEAPP) != 0; }
	[[nodiscard]] bool is_forced_critical() const noexcept     { return (this->lparam & ENDSESSION_CRITICAL) != 0; }
	[[nodiscard]] bool is_logoff() const noexcept              { return (this->lparam & ENDSESSION_LOGOFF) != 0; }
	[[nodiscard]] bool is_shutdown() const noexcept            { return this->lparam == 0; }
};

struct enteridle : public any {
	enteridle(const any& p) noexcept : any{p} { }
	[[nodiscard]] bool is_menu_displayed() const noexcept { return this->wparam == MSGF_MENU; }
	[[nodiscard]] HWND hwindow() const noexcept           { return reinterpret_cast<HWND>(this->lparam); }
};

struct entermenuloop : public any {
	entermenuloop(const any& p) noexcept : any{p} { }
	[[nodiscard]] bool uses_trackpopupmenu() const noexcept { return this->wparam != FALSE; }
};

WINLAMB_EMPTY_WM(entersizemove);

struct erasebkgnd : public any {
	erasebkgnd(const any& p) noexcept : any{p} { }
	[[nodiscard]] HDC hdc() const noexcept { return reinterpret_cast<HDC>(this->wparam); }
};

struct exitmenuloop : public any {
	exitmenuloop(const any& p) noexcept : any{p} { }
	[[nodiscard]] bool is_shortcut_menu() const noexcept { return this->wparam != FALSE; }
};

WINLAMB_EMPTY_WM(exitsizemove);

WINLAMB_EMPTY_WM(fontchange);

struct getdlgcode : public any {
	getdlgcode(const any& p) noexcept : any{p} { }
	[[nodiscard]] BYTE vkey_code() const noexcept { return static_cast<BYTE>(this->wparam); }
	[[nodiscard]] bool is_query() const noexcept  { return this->lparam == 0; }
	[[nodiscard]] const MSG* msg() const noexcept { return this->is_query() ? nullptr : reinterpret_cast<const MSG*>(this->lparam); }
	[[nodiscard]] bool has_alt() const noexcept   { return (GetAsyncKeyState(VK_MENU) & 0x8000) != 0; }
	[[nodiscard]] bool has_ctrl() const noexcept  { return (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0; }
	[[nodiscard]] bool has_shift() const noexcept { return (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0; }
};

WINLAMB_EMPTY_WM(getfont);

WINLAMB_EMPTY_WM(gethotkey);

struct geticon : public any {
	geticon(const any& p) noexcept : any{p} { }
	[[nodiscard]] bool is_big() const noexcept       { return this->wparam == ICON_BIG; }
	[[nodiscard]] bool is_small() const noexcept     { return this->wparam == ICON_SMALL; }
	[[nodiscard]] bool is_small_app() const noexcept { return this->wparam == ICON_SMALL2; }
	[[nodiscard]] UINT dpi() const noexcept          { return static_cast<UINT>(this->lparam); }
};

struct getminmaxinfo : public any {
	getminmaxinfo(const any& p) noexcept : any{p} { }
	[[nodiscard]] MINMAXINFO& minmaxinfo() const noexcept { return *reinterpret_cast<MINMAXINFO*>(this->lparam); }
};

struct gettext : public any {
	gettext(const any& p) noexcept : any{p} { }
	[[nodiscard]] UINT     buffer_size() const noexcept { return static_cast<UINT>(this->wparam); }
	[[nodiscard]] wchar_t* buffer() const noexcept      { return reinterpret_cast<wchar_t*>(this->lparam); }
};

WINLAMB_EMPTY_WM(gettextlength);

struct help : public any {
	help(const any& p) noexcept : any{p} { }
	[[nodiscard]] const HELPINFO& helpinfo() const noexcept { return *reinterpret_cast<const HELPINFO*>(this->lparam); }
};

struct hotkey : public any {
	hotkey(const any& p) noexcept : any{p} { }
	[[nodiscard]] bool is_snap_desktop() const noexcept { return this->wparam == IDHOT_SNAPDESKTOP; }
	[[nodiscard]] bool is_snap_window() const noexcept  { return this->wparam == IDHOT_SNAPWINDOW; }
	[[nodiscard]] bool has_alt() const noexcept         { return (LOWORD(this->lparam) & MOD_ALT) != 0; }
	[[nodiscard]] bool has_ctrl() const noexcept        { return (LOWORD(this->lparam) & MOD_CONTROL) != 0; }
	[[nodiscard]] bool has_shift() const noexcept       { return (LOWORD(this->lparam) & MOD_SHIFT) != 0; }
	[[nodiscard]] bool has_win() const noexcept         { return (LOWORD(this->lparam) & MOD_WIN) != 0; }
	[[nodiscard]] BYTE vkey_code() const noexcept       { return static_cast<BYTE>(HIWORD(this->lparam)); }
};

struct hscroll : public any {
	hscroll(const any& p) noexcept : any{p} { }
	[[nodiscard]] WORD scroll_request() const noexcept { return LOWORD(this->wparam); }
	[[nodiscard]] WORD scroll_pos() const noexcept     { return HIWORD(this->wparam); }
	[[nodiscard]] HWND scrollbar() const noexcept      { return reinterpret_cast<HWND>(this->lparam); }
};

struct hscrollclipboard : public any {
	hscrollclipboard(const any& p) noexcept : any{p} { }
	[[nodiscard]] HWND clipboard_viewer() const noexcept { return reinterpret_cast<HWND>(this->wparam); }
	[[nodiscard]] WORD scroll_event() const noexcept     { return LOWORD(this->lparam); }
	[[nodiscard]] WORD scroll_pos() const noexcept       { return HIWORD(this->lparam); }
};

struct iconerasebkgnd : public any {
	iconerasebkgnd(const any& p) noexcept : any{p} { }
	[[nodiscard]] HDC hdc() const noexcept { return reinterpret_cast<HDC>(this->wparam); }
};

struct initdialog : public any {
	initdialog(const any& p) noexcept : any{p} { }
	[[nodiscard]] HWND focused_ctrl() const noexcept { return reinterpret_cast<HWND>(this->wparam); }
};

struct initmenu : public any {
	initmenu(const any& p) noexcept : any{p} { }
	[[nodiscard]] menu menu() const noexcept { return wl::menu{reinterpret_cast<HMENU>(this->wparam)}; }
};

struct initmenupopup : public any {
	initmenupopup(const any& p) noexcept : any{p} { }
	[[nodiscard]] menu menu() const noexcept               { return wl::menu{reinterpret_cast<HMENU>(this->wparam)}; }
	[[nodiscard]] WORD menu_relative_pos() const noexcept  { return LOWORD(this->lparam); }
	[[nodiscard]] bool is_window_menu() const noexcept     { return HIWORD(this->lparam) != FALSE; }
};

struct inputlangchange : public any {
	inputlangchange(const any& p) noexcept : any{p} { }
	[[nodiscard]] DWORD new_charset() const noexcept     { return static_cast<DWORD>(this->wparam); }
	[[nodiscard]] HKL   keyboard_layout() const noexcept { return reinterpret_cast<HKL>(this->lparam); }
};

struct inputlangchangerequest : public any {
	inputlangchangerequest(const any& p) noexcept : any{p} { }
	[[nodiscard]] bool previous_chosen() const noexcept      { return (this->wparam & INPUTLANGCHANGE_BACKWARD) != 0; }
	[[nodiscard]] bool next_chosen() const noexcept          { return (this->wparam & INPUTLANGCHANGE_FORWARD) != 0; }
	[[nodiscard]] bool can_be_used_with_sys() const noexcept { return (this->wparam & INPUTLANGCHANGE_SYSCHARSET) != 0; }
	[[nodiscard]] HKL  keyboard_layout() const noexcept      { return reinterpret_cast<HKL>(this->lparam); }
};

struct keydown : public any {
	keydown(const any& p) noexcept : any{p} { }
	[[nodiscard]] BYTE vkey_code() const noexcept           { return static_cast<BYTE>(this->wparam); }
	[[nodiscard]] WORD repeat_count() const noexcept        { return LOWORD(this->lparam); }
	[[nodiscard]] BYTE scan_code() const noexcept           { return LOBYTE(HIWORD(this->lparam)); }
	[[nodiscard]] bool is_extended_key() const noexcept     { return (this->lparam >> 24) & 1; }
	[[nodiscard]] bool key_previously_down() const noexcept { return (this->lparam >> 30) & 1; }
};

WINLAMB_INHERIT_WM(keyup, keydown);

struct killfocus : public any {
	killfocus(const any& p) noexcept : any{p} { }
	[[nodiscard]] HWND focused_window() const noexcept { return reinterpret_cast<HWND>(this->wparam); }
};

struct lbuttondblclk : public any {
	lbuttondblclk(const any& p) noexcept : any{p} { }
	[[nodiscard]] bool  has_ctrl() const noexcept       { return (this->wparam & MK_CONTROL) != 0; }
	[[nodiscard]] bool  has_left_btn() const noexcept   { return (this->wparam & MK_LBUTTON) != 0; }
	[[nodiscard]] bool  has_middle_btn() const noexcept { return (this->wparam & MK_MBUTTON) != 0; }
	[[nodiscard]] bool  has_right_btn() const noexcept  { return (this->wparam & MK_RBUTTON) != 0; }
	[[nodiscard]] bool  has_shift() const noexcept      { return (this->wparam & MK_SHIFT) != 0; }
	[[nodiscard]] bool  has_xbtn1() const noexcept      { return (this->wparam & MK_XBUTTON1) != 0; }
	[[nodiscard]] bool  has_xbtn2() const noexcept      { return (this->wparam & MK_XBUTTON2) != 0; }
	[[nodiscard]] POINT pos() const noexcept            { return {GET_X_LPARAM(this->lparam), GET_Y_LPARAM(this->lparam)}; }
};

WINLAMB_INHERIT_WM(lbuttondown, lbuttondblclk);

WINLAMB_INHERIT_WM(lbuttonup, lbuttondblclk);

WINLAMB_INHERIT_WM(mbuttondblclk, lbuttondblclk);

WINLAMB_INHERIT_WM(mbuttondown, lbuttondblclk);

WINLAMB_INHERIT_WM(mbuttonup, lbuttondblclk);

struct mdiactivate : public any {
	mdiactivate(const any& p) noexcept : any{p} { }
	[[nodiscard]] HWND activated_child() const noexcept   { return reinterpret_cast<HWND>(this->wparam); }
	[[nodiscard]] HWND deactivated_child() const noexcept { return reinterpret_cast<HWND>(this->lparam); }
};

struct measureitem : public any {
	measureitem(const any& p) noexcept : any{p} { }
	[[nodiscard]] const MEASUREITEMSTRUCT& measureitemstruct() const noexcept { return *reinterpret_cast<const MEASUREITEMSTRUCT*>(this->lparam); }
};

struct menuchar : public any {
	menuchar(const any& p) noexcept : any{p} { }
	[[nodiscard]] WORD char_code() const noexcept      { return LOWORD(this->wparam); }
	[[nodiscard]] bool is_window_menu() const noexcept { return HIWORD(this->wparam) == MF_SYSMENU; }
	[[nodiscard]] menu menu() const noexcept           { return wl::menu{reinterpret_cast<HMENU>(this->lparam)}; }
};

struct menudrag : public any {
	menudrag(const any& p) noexcept : any{p} { }
	[[nodiscard]] UINT initial_pos() const noexcept { return static_cast<UINT>(this->wparam); }
	[[nodiscard]] menu menu() const noexcept        { return wl::menu{reinterpret_cast<HMENU>(this->lparam)}; }
};

struct menugetobject : public any {
	menugetobject(const any& p) noexcept : any{p} { }
	[[nodiscard]] const MENUGETOBJECTINFO& menugetobjectinfo() const noexcept { return *reinterpret_cast<const MENUGETOBJECTINFO*>(this->lparam); }
};

struct menurbuttonup : public any {
	menurbuttonup(const any& p) noexcept : any{p} { }
	[[nodiscard]] UINT index() const noexcept { return static_cast<UINT>(this->wparam); }
	[[nodiscard]] menu menu() const noexcept  { return wl::menu{reinterpret_cast<HMENU>(this->lparam)}; }
};

struct menuselect : public any {
	menuselect(const any& p) noexcept : any{p} { }
	[[nodiscard]] WORD item() const noexcept              { return LOWORD(this->wparam); }
	[[nodiscard]] bool has_bitmap() const noexcept        { return (HIWORD(this->wparam) & MF_BITMAP) != 0; }
	[[nodiscard]] bool is_checked() const noexcept        { return (HIWORD(this->wparam) & MF_CHECKED) != 0; }
	[[nodiscard]] bool is_disabled() const noexcept       { return (HIWORD(this->wparam) & MF_DISABLED) != 0; }
	[[nodiscard]] bool is_grayed() const noexcept         { return (HIWORD(this->wparam) & MF_GRAYED) != 0; }
	[[nodiscard]] bool is_highlighted() const noexcept    { return (HIWORD(this->wparam) & MF_HILITE) != 0; }
	[[nodiscard]] bool mouse_selected() const noexcept    { return (HIWORD(this->wparam) & MF_MOUSESELECT) != 0; }
	[[nodiscard]] bool is_owner_draw() const noexcept     { return (HIWORD(this->wparam) & MF_OWNERDRAW) != 0; }
	[[nodiscard]] bool opens_popup() const noexcept       { return (HIWORD(this->wparam) & MF_POPUP) != 0; }
	[[nodiscard]] bool is_sysmenu() const noexcept        { return (HIWORD(this->wparam) & MF_SYSMENU) != 0; }
	[[nodiscard]] bool system_has_closed() const noexcept { return HIWORD(this->wparam) == 0xFFFF && !this->lparam; }
	[[nodiscard]] menu menu() const noexcept              { return wl::menu{this->opens_popup() || this->is_sysmenu() ? reinterpret_cast<HMENU>(this->lparam) : nullptr}; }
};

struct mouseactivate : public any {
	mouseactivate(const any& p) noexcept : any{p} { }
	[[nodiscard]] short hit_test_code() const noexcept { return static_cast<short>(LOWORD(this->lparam)); }
	[[nodiscard]] WORD  mouse_msg_id() const noexcept  { return HIWORD(this->lparam); }
};

WINLAMB_INHERIT_WM(mousehover, lbuttondblclk);

WINLAMB_EMPTY_WM(mouseleave);

WINLAMB_INHERIT_WM(mousemove, lbuttondblclk);

struct mousewheel : public any {
	mousewheel(const any& p) noexcept : any{p} { }
	[[nodiscard]] short wheel_delta() const noexcept    { return GET_WHEEL_DELTA_WPARAM(this->wparam); }
	[[nodiscard]] bool  has_ctrl() const noexcept       { return (LOWORD(this->wparam) & MK_CONTROL) != 0; }
	[[nodiscard]] bool  has_left_btn() const noexcept   { return (LOWORD(this->wparam) & MK_LBUTTON) != 0; }
	[[nodiscard]] bool  has_middle_btn() const noexcept { return (LOWORD(this->wparam) & MK_MBUTTON) != 0; }
	[[nodiscard]] bool  has_right_btn() const noexcept  { return (LOWORD(this->wparam) & MK_RBUTTON) != 0; }
	[[nodiscard]] bool  has_shift() const noexcept      { return (LOWORD(this->wparam) & MK_SHIFT) != 0; }
	[[nodiscard]] bool  has_xbtn1() const noexcept      { return (LOWORD(this->wparam) & MK_XBUTTON1) != 0; }
	[[nodiscard]] bool  has_xbtn2() const noexcept      { return (LOWORD(this->wparam) & MK_XBUTTON2) != 0; }
	[[nodiscard]] POINT pos() const noexcept            { return {GET_X_LPARAM(this->lparam), GET_Y_LPARAM(this->lparam)}; }
};

struct move : public any {
	move(const any& p) noexcept : any{p} { }
	[[nodiscard]] POINT pos() const noexcept { return {GET_X_LPARAM(this->lparam), GET_Y_LPARAM(this->lparam)}; }
};

struct moving : public any {
	moving(const any& p) noexcept : any{p} { }
	[[nodiscard]] RECT& screen_coords() const noexcept { return *reinterpret_cast<RECT*>(this->lparam); }
};

struct ncactivate : public any {
	ncactivate(const any& p) noexcept : any{p} { }
	[[nodiscard]] bool is_active() const noexcept { return this->wparam == TRUE; }
};

struct nccalcsize : public any {
	nccalcsize(const any& p) noexcept : any{p} { }
	[[nodiscard]] bool                     is_nccalcsize() const noexcept     { return this->wparam == TRUE; }
	[[nodiscard]] bool                     is_rect() const noexcept           { return this->wparam == FALSE; }
	[[nodiscard]] const NCCALCSIZE_PARAMS& nccalcsize_params() const noexcept { return *reinterpret_cast<const NCCALCSIZE_PARAMS*>(this->lparam); }
	[[nodiscard]] const RECT&              rect() const noexcept              { return *reinterpret_cast<const RECT*>(this->lparam); }
};

WINLAMB_INHERIT_WM(nccreate, create);

WINLAMB_EMPTY_WM(ncdestroy);

struct nchittest : public any {
	nchittest(const any& p) noexcept : any{p} { }
	[[nodiscard]] POINT pos() const noexcept { return {GET_X_LPARAM(this->lparam), GET_Y_LPARAM(this->lparam)}; }
};

struct nclbuttondblclk : public any {
	nclbuttondblclk(const any& p) noexcept : any{p} { }
	[[nodiscard]] short hit_test_code() const noexcept { return static_cast<short>(this->wparam); }
	[[nodiscard]] POINT pos() const noexcept           { return {GET_X_LPARAM(this->lparam), GET_Y_LPARAM(this->lparam)}; }
};

WINLAMB_INHERIT_WM(nclbuttondown, nclbuttondblclk);

WINLAMB_INHERIT_WM(nclbuttonup, nclbuttondblclk);

WINLAMB_INHERIT_WM(ncmbuttondblclk, nclbuttondblclk);

WINLAMB_INHERIT_WM(ncmbuttondown, nclbuttondblclk);

WINLAMB_INHERIT_WM(ncmbuttonup, nclbuttondblclk);

WINLAMB_INHERIT_WM(ncmousemove, nclbuttondblclk);

WINLAMB_INHERIT_WM(ncrbuttondblclk, nclbuttondblclk);

WINLAMB_INHERIT_WM(ncrbuttondown, nclbuttondblclk);

WINLAMB_INHERIT_WM(ncrbuttonup, nclbuttondblclk);

struct ncpaint : public any {
	ncpaint(const any& p) noexcept : any{p} { }
	[[nodiscard]] HRGN hrgn() const noexcept { return reinterpret_cast<HRGN>(this->wparam); }
};

struct nextdlgctl : public any {
	nextdlgctl(const any& p) noexcept : any{p} { }
	[[nodiscard]] bool has_ctrl_receiving_focus() const noexcept { return LOWORD(this->lparam) != FALSE; }
	[[nodiscard]] HWND ctrl_receiving_focus() const noexcept     { return LOWORD(this->lparam) ? reinterpret_cast<HWND>(this->wparam) : nullptr; }
	[[nodiscard]] bool focus_next() const noexcept               { return this->wparam == 0; }
};

struct nextmenu : public any {
	nextmenu(const any& p) noexcept : any{p} { }
	[[nodiscard]] BYTE               vkey_code() const noexcept   { return static_cast<BYTE>(this->wparam); }
	[[nodiscard]] const MDINEXTMENU& mdinextmenu() const noexcept { return *reinterpret_cast<const MDINEXTMENU*>(this->lparam); }
};

struct notify : public any {
	notify(const any& p) noexcept : any{p} { }
	[[nodiscard]] const NMHDR& nmhdr() const noexcept { return *reinterpret_cast<const NMHDR*>(this->lparam); }
};

struct notifyformat : public any {
	notifyformat(const any& p) noexcept : any{p} { }
	[[nodiscard]] HWND window_from() const noexcept           { return reinterpret_cast<HWND>(this->wparam); }
	[[nodiscard]] bool is_query_from_control() const noexcept { return this->lparam == NF_QUERY; }
	[[nodiscard]] bool is_requery_to_control() const noexcept { return this->lparam == NF_REQUERY; }
};

WINLAMB_EMPTY_WM(paint);

struct paintclipboard : public any {
	paintclipboard(const any& p) noexcept : any{p} { }
	[[nodiscard]] HWND               clipboard_viewer() const noexcept { return reinterpret_cast<HWND>(this->wparam); }
	[[nodiscard]] const PAINTSTRUCT& paintstruct() const noexcept      { return *reinterpret_cast<const PAINTSTRUCT*>(this->lparam); }
};

struct palettechanged : public any {
	palettechanged(const any& p) noexcept : any{p} { }
	[[nodiscard]] HWND window_origin() const noexcept { return reinterpret_cast<HWND>(this->wparam); }
};

struct paletteischanging : public any {
	paletteischanging(const any& p) noexcept : any{p} { }
	[[nodiscard]] HWND window_origin() const noexcept { return reinterpret_cast<HWND>(this->wparam); }
};

struct parentnotify : public any {
	parentnotify(const any& p) noexcept : any{p} { }
	[[nodiscard]] UINT  event_message() const noexcept { return static_cast<UINT>(LOWORD(this->wparam)); }
	[[nodiscard]] WORD  child_id() const noexcept      { return HIWORD(this->wparam); }
	[[nodiscard]] HWND  child_hwnd() const noexcept    { return reinterpret_cast<HWND>(this->lparam); }
	[[nodiscard]] POINT pos() const noexcept           { return {GET_X_LPARAM(this->lparam), GET_Y_LPARAM(this->lparam)}; }
	[[nodiscard]] bool  is_xbutton1() const noexcept   { return HIWORD(this->wparam) == XBUTTON1; }
	[[nodiscard]] WORD  pointer_flag() const noexcept  { return HIWORD(this->wparam); }
};

struct powerbroadcast : public any {
	powerbroadcast(const any& p) noexcept : any{p} { }
	[[nodiscard]] bool                          is_power_status_change() const noexcept  { return this->wparam == PBT_APMPOWERSTATUSCHANGE; }
	[[nodiscard]] bool                          is_resuming() const noexcept             { return this->wparam == PBT_APMRESUMEAUTOMATIC; }
	[[nodiscard]] bool                          is_suspending() const noexcept           { return this->wparam == PBT_APMSUSPEND; }
	[[nodiscard]] bool                          is_power_setting_change() const noexcept { return this->wparam == PBT_POWERSETTINGCHANGE; }
	[[nodiscard]] const POWERBROADCAST_SETTING& power_setting() const noexcept           { return *reinterpret_cast<const POWERBROADCAST_SETTING*>(this->lparam); }
};

struct print : public any {
	print(const any& p) noexcept : any{p} { }
	[[nodiscard]] HDC  hdc() const noexcept   { return reinterpret_cast<HDC>(this->wparam); }
	[[nodiscard]] UINT flags() const noexcept { return static_cast<UINT>(this->lparam); }
};

struct printclient : public any {
	printclient(const any& p) noexcept : any{p} { }
	[[nodiscard]] HDC  hdc() const noexcept   { return reinterpret_cast<HDC>(this->wparam); }
	[[nodiscard]] UINT flags() const noexcept { return static_cast<UINT>(this->lparam); }
};

WINLAMB_EMPTY_WM(querydragicon);

struct queryendsession : public any {
	queryendsession(const any& p) noexcept : any{p} { }
	[[nodiscard]] bool is_system_issue() const noexcept    { return (this->lparam & ENDSESSION_CLOSEAPP) != 0; }
	[[nodiscard]] bool is_forced_critical() const noexcept { return (this->lparam & ENDSESSION_CRITICAL) != 0; }
	[[nodiscard]] bool is_logoff() const noexcept          { return (this->lparam & ENDSESSION_LOGOFF) != 0; }
	[[nodiscard]] bool is_shutdown() const noexcept        { return this->lparam == 0; }
};

WINLAMB_EMPTY_WM(querynewpalette);

WINLAMB_EMPTY_WM(queryopen);

#ifdef _RAS_H_ // Ras.h
struct rasdialevent : public wplp {
	rasdialevent(const wplp& p) : wplp(p) { }
	[[nodiscard]] RASCONNSTATE rasconnstate() const noexcept { return static_cast<RASCONNSTATE>(this->wparam); }
	[[nodiscard]] DWORD        error() const noexcept        { return static_cast<DWORD>(this->lparam); }
};
#endif

WINLAMB_INHERIT_WM(rbuttondblclk, lbuttondblclk);

WINLAMB_INHERIT_WM(rbuttondown, lbuttondblclk);

WINLAMB_INHERIT_WM(rbuttonup, lbuttondblclk);

WINLAMB_EMPTY_WM(renderallformats);

struct renderformat : public any {
	renderformat(const any& p) noexcept : any{p} { }
	[[nodiscard]] WORD clipboard_format() const noexcept { return static_cast<WORD>(this->wparam); }
};

struct setcursor : public any {
	setcursor(const any& p) noexcept : any{p} { }
	[[nodiscard]] HWND  cursor_owner() const noexcept  { return reinterpret_cast<HWND>(this->wparam); }
	[[nodiscard]] short hit_test_code() const noexcept { return static_cast<short>(LOWORD(this->lparam)); }
	[[nodiscard]] WORD  mouse_msg_id() const noexcept  { return HIWORD(this->lparam); }
};

struct setfocus : public any {
	setfocus(const any& p) noexcept : any{p} { }
	[[nodiscard]] HWND unfocused_window() const noexcept { return reinterpret_cast<HWND>(this->wparam); }
};

struct setfont : public any {
	setfont(const any& p) noexcept : any{p} { }
	[[nodiscard]] HFONT hfont() const noexcept   { return reinterpret_cast<HFONT>(this->wparam); }
	[[nodiscard]] bool  should_redraw() noexcept { return LOWORD(this->lparam) != FALSE; }
};

struct sethotkey : public any {
	sethotkey(const any& p) noexcept : any{p} { }
	[[nodiscard]] BYTE vkey_code() const noexcept    { return static_cast<BYTE>(LOWORD(this->wparam)); }
	[[nodiscard]] bool has_alt() const noexcept      { return (HIWORD(this->wparam) & 0x04) != 0; }
	[[nodiscard]] bool has_ctrl() const noexcept     { return (HIWORD(this->wparam) & 0x02) != 0; }
	[[nodiscard]] bool has_extended() const noexcept { return (HIWORD(this->wparam) & 0x08) != 0; }
	[[nodiscard]] bool has_shift() const noexcept    { return (HIWORD(this->wparam) & 0x01) != 0; }
};

struct seticon : public any {
	seticon(const any& p) noexcept : any{p} { }
	[[nodiscard]] bool  is_small() const noexcept   { return this->wparam == ICON_SMALL; }
	[[nodiscard]] HICON hicon() const noexcept      { return reinterpret_cast<HICON>(this->lparam); }
	[[nodiscard]] bool  is_removed() const noexcept { return this->hicon() == nullptr; }
};

struct setredraw : public any {
	setredraw(const any& p) noexcept : any{p} { }
	[[nodiscard]] bool can_redraw() const noexcept { return this->wparam != FALSE; }
};

struct settext : public any {
	settext(const any& p) noexcept : any{p} { }
	[[nodiscard]] const wchar_t* text() const noexcept { return reinterpret_cast<const wchar_t*>(this->lparam); }
};

struct settingchange : public any {
	settingchange(const any& p) noexcept : any{p} { }
	[[nodiscard]] const wchar_t* string_id() const noexcept           { return reinterpret_cast<const wchar_t*>(this->lparam); }
	[[nodiscard]] bool           is_policy() const noexcept           { return !lstrcmpW(this->string_id(), L"Policy"); }
	[[nodiscard]] bool           is_locale() const noexcept           { return !lstrcmpW(this->string_id(), L"intl"); }
	[[nodiscard]] bool           is_environment_vars() const noexcept { return !lstrcmpW(this->string_id(), L"Environment"); }
};

struct showwindow : public any {
	showwindow(const any& p) noexcept : any{p} { }
	[[nodiscard]] bool is_being_shown() const noexcept           { return this->wparam != FALSE; }
	[[nodiscard]] bool is_other_away() const noexcept            { return this->lparam == SW_OTHERUNZOOM; }
	[[nodiscard]] bool is_other_over() const noexcept            { return this->lparam == SW_OTHERZOOM; }
	[[nodiscard]] bool is_owner_being_minimized() const noexcept { return this->lparam == SW_PARENTCLOSING; }
	[[nodiscard]] bool is_owner_being_restored() const noexcept  { return this->lparam == SW_PARENTOPENING; }
};

struct size : public any {
	size(const any& p) noexcept : any{p} { }
	[[nodiscard]] bool is_other_maximized() const noexcept { return this->wparam == SIZE_MAXHIDE; }
	[[nodiscard]] bool is_maximized() const noexcept       { return this->wparam == SIZE_MAXIMIZED; }
	[[nodiscard]] bool is_other_restored() const noexcept  { return this->wparam == SIZE_MAXSHOW; }
	[[nodiscard]] bool is_minimized() const noexcept       { return this->wparam == SIZE_MINIMIZED; }
	[[nodiscard]] bool is_restored() const noexcept        { return this->wparam == SIZE_RESTORED; }
	[[nodiscard]] SIZE sz() const noexcept                 { return {LOWORD(this->lparam), HIWORD(this->lparam)}; }
};

struct sizeclipboard : public any {
	sizeclipboard(const any& p) noexcept : any{p} { }
	[[nodiscard]] HWND        clipboard_viewer() const noexcept { return reinterpret_cast<HWND>(this->wparam); }
	[[nodiscard]] const RECT& clipboard_rect() const noexcept   { return *reinterpret_cast<const RECT*>(this->lparam); }
};

struct sizing : public any {
	sizing(const any& p) noexcept : any{p} { }
	[[nodiscard]] WORD  edge() const noexcept          { return static_cast<WORD>(this->wparam); }
	[[nodiscard]] RECT& screen_coords() const noexcept { return *reinterpret_cast<RECT*>(this->lparam); }
};

struct spoolerstatus : public any {
	spoolerstatus(const any& p) noexcept : any{p} { }
	[[nodiscard]] UINT status_flag() const noexcept    { return static_cast<UINT>(this->wparam); }
	[[nodiscard]] WORD remaining_jobs() const noexcept { return LOWORD(this->lparam); }
};

struct stylechanged : public any {
	stylechanged(const any& p) noexcept : any{p} { }
	[[nodiscard]] bool               is_style() const noexcept    { return (this->wparam & GWL_STYLE) != 0; }
	[[nodiscard]] bool               is_ex_style() const noexcept { return (this->wparam & GWL_EXSTYLE) != 0; }
	[[nodiscard]] const STYLESTRUCT& stylestruct() const noexcept { return *reinterpret_cast<const STYLESTRUCT*>(this->lparam); }
};

WINLAMB_INHERIT_WM(stylechanging, stylechanged);

WINLAMB_INHERIT_WM(syschar, char_);

struct syscommand : public any {
	syscommand(const any& p) noexcept : any{p} { }
	[[nodiscard]] WORD  command_type() const noexcept { return static_cast<WORD>(this->wparam); }
	[[nodiscard]] POINT pos() const noexcept          { return {GET_X_LPARAM(this->lparam), GET_Y_LPARAM(this->lparam)}; }
};

WINLAMB_INHERIT_WM(sysdeadchar, char_);

WINLAMB_INHERIT_WM(syskeydown, keydown);

WINLAMB_INHERIT_WM(syskeyup, keydown);

struct tcard : public any {
	tcard(const any& p) noexcept : any{p} { }
	[[nodiscard]] UINT action_id() const noexcept   { return static_cast<UINT>(this->wparam); }
	[[nodiscard]] long action_data() const noexcept { return static_cast<long>(this->lparam); }
};

WINLAMB_EMPTY_WM(timechange);

struct timer : public any {
	timer(const any& p) noexcept : any{p} { }
	[[nodiscard]] UINT_PTR  timer_id() const noexcept { return static_cast<UINT_PTR>(this->wparam); }
	[[nodiscard]] TIMERPROC callback() const noexcept { return reinterpret_cast<TIMERPROC>(this->lparam); }
};

struct uninitmenupopup : public any {
	uninitmenupopup(const any& p) noexcept : any{p} { }
	[[nodiscard]] menu menu() const noexcept    { return wl::menu{reinterpret_cast<HMENU>(this->wparam)}; }
	[[nodiscard]] WORD menu_id() const noexcept { return HIWORD(this->lparam); }
};

WINLAMB_EMPTY_WM(userchanged);

struct vkeytoitem : public any {
	vkeytoitem(const any& p) noexcept : any{p} { }
	[[nodiscard]] BYTE vkey_code() const noexcept         { return static_cast<BYTE>(LOWORD(this->wparam)); }
	[[nodiscard]] WORD current_caret_pos() const noexcept { return HIWORD(this->wparam); }
	[[nodiscard]] HWND hlistbox() const noexcept          { return reinterpret_cast<HWND>(this->lparam); }
};

WINLAMB_INHERIT_WM(vscroll, hscroll);

WINLAMB_INHERIT_WM(vscrollclipboard, hscrollclipboard);

struct windowposchanged : public any {
	windowposchanged(const any& p) noexcept : any{p} { }
	[[nodiscard]] const WINDOWPOS& windowpos() const noexcept { return *reinterpret_cast<const WINDOWPOS*>(this->lparam); }
};

WINLAMB_INHERIT_WM(windowposchanging, windowposchanged);

}//namespace wl::param::wm

#define WINLAMB_NOTIFY_WM(structN, objN) \
	struct structN : public wm::notify { \
		structN(const any& p) noexcept : notify{p} { } \
		[[nodiscard]] const objN& nmhdr() const noexcept { return *reinterpret_cast<const objN*>(this->lparam); } \
	};

// Message crackers for extended combo box notifications.
namespace wl::param::cben {

WINLAMB_NOTIFY_WM(beginedit, NMHDR);
WINLAMB_NOTIFY_WM(deleteitem, NMCOMBOBOXEXW);
WINLAMB_NOTIFY_WM(dragbegin, NMCBEDRAGBEGINW);
WINLAMB_NOTIFY_WM(endedit, NMCBEENDEDITW);
WINLAMB_NOTIFY_WM(getdispinfo, NMCOMBOBOXEXW);
WINLAMB_NOTIFY_WM(insertitem, NMCOMBOBOXEXW);
WINLAMB_NOTIFY_WM(setcursor, NMMOUSE);

}//namespace wl::param::cben

// Message crackers for date and time picker notifications.
namespace wl::param::dtn {

WINLAMB_NOTIFY_WM(closeup, NMHDR);
WINLAMB_NOTIFY_WM(datetimechange, NMDATETIMECHANGE);
WINLAMB_NOTIFY_WM(dropdown, NMHDR);
WINLAMB_NOTIFY_WM(format, NMDATETIMEFORMATW);
WINLAMB_NOTIFY_WM(formatquery, NMDATETIMEFORMATQUERYW);
WINLAMB_NOTIFY_WM(userstring, NMDATETIMESTRINGW);
WINLAMB_NOTIFY_WM(wmkeydown, NMDATETIMEWMKEYDOWNW);
WINLAMB_NOTIFY_WM(killfocus, NMHDR);
WINLAMB_NOTIFY_WM(setfocus, NMHDR);

}//namespace wl::param::dtn

// Message crackers for list view notifications.
namespace wl::param::lvn {

WINLAMB_NOTIFY_WM(begindrag, NMLISTVIEW);
WINLAMB_NOTIFY_WM(beginlabeledit, NMLVDISPINFOW);
WINLAMB_NOTIFY_WM(beginrdrag, NMLISTVIEW);
WINLAMB_NOTIFY_WM(beginscroll, NMLVSCROLL);
WINLAMB_NOTIFY_WM(columnclick, NMLISTVIEW);
WINLAMB_NOTIFY_WM(columndropdown, NMLISTVIEW);
WINLAMB_NOTIFY_WM(columnoverflowclick, NMLISTVIEW);
WINLAMB_NOTIFY_WM(deleteallitems, NMLISTVIEW);
WINLAMB_NOTIFY_WM(deleteitem, NMLISTVIEW);
WINLAMB_NOTIFY_WM(endlabeledit, NMLVDISPINFOW);
WINLAMB_NOTIFY_WM(endscroll, NMLVSCROLL);
WINLAMB_NOTIFY_WM(getdispinfo, NMLVDISPINFOW);
WINLAMB_NOTIFY_WM(getemptymarkup, NMLVEMPTYMARKUP);
WINLAMB_NOTIFY_WM(getinfotip, NMLVGETINFOTIPW);
WINLAMB_NOTIFY_WM(hottrack, NMLISTVIEW);
WINLAMB_NOTIFY_WM(incrementalsearch, NMLVFINDITEMW);
WINLAMB_NOTIFY_WM(insertitem, NMLISTVIEW);
WINLAMB_NOTIFY_WM(itemactivate, NMITEMACTIVATE);
WINLAMB_NOTIFY_WM(itemchanged, NMLISTVIEW);
WINLAMB_NOTIFY_WM(itemchanging, NMLISTVIEW);
WINLAMB_NOTIFY_WM(keydown, NMLVKEYDOWN);
WINLAMB_NOTIFY_WM(linkclick, NMLVLINK);
WINLAMB_NOTIFY_WM(marqueebegin, NMHDR);
WINLAMB_NOTIFY_WM(odcachehint, NMLVCACHEHINT);
WINLAMB_NOTIFY_WM(odfinditem, NMLVFINDITEMW);
WINLAMB_NOTIFY_WM(odstatechanged, NMLVODSTATECHANGE);
WINLAMB_NOTIFY_WM(setdispinfo, NMLVDISPINFOW);
WINLAMB_NOTIFY_WM(click, NMITEMACTIVATE);
WINLAMB_NOTIFY_WM(customdraw, NMLVCUSTOMDRAW);
WINLAMB_NOTIFY_WM(dblclk, NMITEMACTIVATE);
WINLAMB_NOTIFY_WM(hover, NMHDR);
WINLAMB_NOTIFY_WM(killfocus, NMHDR);
WINLAMB_NOTIFY_WM(rclick, NMITEMACTIVATE);
WINLAMB_NOTIFY_WM(rdblclk, NMITEMACTIVATE);
WINLAMB_NOTIFY_WM(releasedcapture, NMHDR);
WINLAMB_NOTIFY_WM(return_, NMHDR);
WINLAMB_NOTIFY_WM(setfocus, NMHDR);

}//namespace wl::param::lvn

// Message crackers for month calendar notifications.
namespace wl::param::mcn {

WINLAMB_NOTIFY_WM(getdaystate, NMDAYSTATE);
WINLAMB_NOTIFY_WM(selchange, NMSELCHANGE);
WINLAMB_NOTIFY_WM(select, NMSELCHANGE);
WINLAMB_NOTIFY_WM(viewchange, NMVIEWCHANGE);
WINLAMB_NOTIFY_WM(releasedcapture, NMHDR);

}//namespace wl::param::mcn

// Message crackers for status bar notifications.
namespace wl::param::sbn {

WINLAMB_NOTIFY_WM(simplemodechange, NMHDR);
WINLAMB_NOTIFY_WM(click, NMMOUSE);
WINLAMB_NOTIFY_WM(dblclk, NMMOUSE);
WINLAMB_NOTIFY_WM(rclick, NMMOUSE);
WINLAMB_NOTIFY_WM(rdblclk, NMMOUSE);

}//nampesace wl::param::sbn

// Message crackers for tab control notifications.
namespace wl::param::tcn {

WINLAMB_NOTIFY_WM(focuschange, NMHDR);
WINLAMB_NOTIFY_WM(getobject, NMOBJECTNOTIFY);
WINLAMB_NOTIFY_WM(keydown, NMTCKEYDOWN);
WINLAMB_NOTIFY_WM(selchange, NMHDR);
WINLAMB_NOTIFY_WM(selchanging, NMHDR);
WINLAMB_NOTIFY_WM(click, NMHDR);
WINLAMB_NOTIFY_WM(dblclk, NMHDR);
WINLAMB_NOTIFY_WM(rclick, NMHDR);
WINLAMB_NOTIFY_WM(rdblclk, NMHDR);
WINLAMB_NOTIFY_WM(releasedcapture, NMHDR);

}//namespace wl::param::tcn

// Message crackers for trackbar notifications.
namespace wl::param::trbn {

WINLAMB_NOTIFY_WM(thumbposchanging, NMTRBTHUMBPOSCHANGING);
WINLAMB_NOTIFY_WM(customdraw, NMCUSTOMDRAW);
WINLAMB_NOTIFY_WM(releasedcapture, NMHDR);

}//namespace wl::param::trbn

// Message crackers fo tooltip notifications.
namespace wl::param::ttn {

WINLAMB_NOTIFY_WM(getdispinfo, NMTTDISPINFOW);
WINLAMB_NOTIFY_WM(linkclick, NMHDR);
WINLAMB_NOTIFY_WM(needtext, NMTTDISPINFOW);
WINLAMB_NOTIFY_WM(pop, NMHDR);
WINLAMB_NOTIFY_WM(show, NMHDR);
WINLAMB_NOTIFY_WM(customdraw, NMTTCUSTOMDRAW);

}//namespace wl::param::ttn

// Message crackers for tree view notifications.
namespace wl::param::tvn {

WINLAMB_NOTIFY_WM(asyncdraw, NMTVASYNCDRAW);
WINLAMB_NOTIFY_WM(begindrag, NMTREEVIEWW);
WINLAMB_NOTIFY_WM(beginlabeledit, NMTVDISPINFOW);
WINLAMB_NOTIFY_WM(beginrdrag, NMTREEVIEWW);
WINLAMB_NOTIFY_WM(deleteitem, NMTREEVIEWW);
WINLAMB_NOTIFY_WM(endlabeledit, NMTVDISPINFOW);
WINLAMB_NOTIFY_WM(getdispinfo, NMTVDISPINFOW);
WINLAMB_NOTIFY_WM(getinfotip, NMTVGETINFOTIPW);
WINLAMB_NOTIFY_WM(itemchanged, NMTVITEMCHANGE);
WINLAMB_NOTIFY_WM(itemchanging, NMTVITEMCHANGE);
WINLAMB_NOTIFY_WM(itemexpanded, NMTREEVIEWW);
WINLAMB_NOTIFY_WM(itemexpanding, NMTREEVIEWW);
WINLAMB_NOTIFY_WM(keydown, NMTVKEYDOWN);
WINLAMB_NOTIFY_WM(selchanged, NMTREEVIEWW);
WINLAMB_NOTIFY_WM(selchanging, NMTREEVIEWW);
WINLAMB_NOTIFY_WM(setdispinfo, NMTVDISPINFOW);
WINLAMB_NOTIFY_WM(singleexpand, NMTREEVIEWW);
WINLAMB_NOTIFY_WM(click, NMHDR);
WINLAMB_NOTIFY_WM(customdraw, NMTVCUSTOMDRAW);
WINLAMB_NOTIFY_WM(dblclk, NMHDR);
WINLAMB_NOTIFY_WM(killfocus, NMHDR);
WINLAMB_NOTIFY_WM(rclick, NMHDR);
WINLAMB_NOTIFY_WM(rdblclk, NMHDR);
WINLAMB_NOTIFY_WM(return_, NMHDR);
WINLAMB_NOTIFY_WM(setcursor, NMMOUSE);
WINLAMB_NOTIFY_WM(setfocus, NMHDR);

}//namespace wl::param::tvn

// Message crackers for up-down control notifications.
namespace wl::param::udn {

WINLAMB_NOTIFY_WM(deltapos, NMUPDOWN);
WINLAMB_NOTIFY_WM(releasedcapture, NMHDR);

}//namespace wl::param::udn