/**
 * Part of WinLamb - Win32 API Lambda Library
 * https://github.com/rodrigocfd/winlamb
 * This library is released under the MIT License
 */

#pragma once
#include <algorithm>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <system_error>
#include <vector>
#include <Windows.h>
#include <Shlobj.h>
#include "internals/interfaces.h"
#include "internals/sys_dlg_aux.h"
#include "com.h"

// Native system dialogs.
namespace wl::sys_dlg {

// Ordinary MessageBox, but centered at parent.
inline int msg_box(const i_window* parent,
	std::wstring_view title, std::wstring_view text, UINT uType = 0)
{
	if (!parent) {
		throw std::logic_error("Cannot call msg_box without a parent.");
	}

	_wli::globalMsgBoxParent = parent->hwnd();

	_wli::globalMsgBoxHook = SetWindowsHookExW(WH_CBT,
		[](int code, WPARAM wp, LPARAM lp) noexcept -> LRESULT {
			// http://www.codeguru.com/cpp/w-p/win32/messagebox/print.php/c4541
			if (code == HCBT_ACTIVATE) {
				HWND hMsgbox = reinterpret_cast<HWND>(wp);
				RECT rcMsgbox{}, rcParent{};

				if (hMsgbox &&
					_wli::globalMsgBoxParent &&
					GetWindowRect(hMsgbox, &rcMsgbox) &&
					GetWindowRect(_wli::globalMsgBoxParent, &rcParent))
				{
					RECT  rcScreen{};
					POINT pos{};
					SystemParametersInfoW(SPI_GETWORKAREA, 0,
						static_cast<PVOID>(&rcScreen), 0); // size of desktop

					// Adjusted x,y coordinates to message box window.
					pos.x = rcParent.left + (rcParent.right - rcParent.left)
						/ 2 - (rcMsgbox.right - rcMsgbox.left) / 2;
					pos.y = rcParent.top + (rcParent.bottom - rcParent.top)
						/ 2 - (rcMsgbox.bottom - rcMsgbox.top) / 2;

					// Screen out-of-bounds corrections.
					if (pos.x < 0) {
						pos.x = 0;
					} else if (pos.x + (rcMsgbox.right - rcMsgbox.left) > rcScreen.right) {
						pos.x = rcScreen.right - (rcMsgbox.right - rcMsgbox.left);
					}
					if (pos.y < 0) {
						pos.y = 0;
					} else if (pos.y + (rcMsgbox.bottom - rcMsgbox.top) > rcScreen.bottom) {
						pos.y = rcScreen.bottom - (rcMsgbox.bottom - rcMsgbox.top);
					}

					MoveWindow(hMsgbox, pos.x, pos.y,
						rcMsgbox.right - rcMsgbox.left, rcMsgbox.bottom - rcMsgbox.top,
						FALSE);
				}
				UnhookWindowsHookEx(_wli::globalMsgBoxHook); // release global hook
			}
			return CallNextHookEx(nullptr, code, wp, lp);
		},
	nullptr, GetCurrentThreadId());

	if (!_wli::globalMsgBoxHook) {
		throw std::system_error(GetLastError(), std::system_category(),
			"SetWindowsHookEx failed for msg_box");
	}

	return MessageBoxW(parent->hwnd(), text.data(), title.data(), uType);
}

// File open dialog for a single file.
// Uses C# filter syntax:
// L"Text Files (*.txt)|*.txt|All Files (*.*)|*.*"
[[nodiscard]] inline std::optional<std::wstring>
	open_file(const i_window* parent, std::wstring_view filterWithPipes) noexcept
{
	OPENFILENAME         ofn{};
	wchar_t              tmpBuf[MAX_PATH]{};
	std::vector<wchar_t> multiZFilter = _wli::format_file_filter(filterWithPipes);

	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner   = parent->hwnd();
	ofn.lpstrFilter = &multiZFilter[0];
	ofn.lpstrFile   = tmpBuf;
	ofn.nMaxFile    = ARRAYSIZE(tmpBuf);
	ofn.Flags       = OFN_EXPLORER | OFN_ENABLESIZING | OFN_FILEMUSTEXIST;// | OFN_HIDEREADONLY;

	if (GetOpenFileNameW(&ofn)) {
		return tmpBuf;
	}
	return {};
}

// File open dialog for multiple files.
// Uses C# filter syntax:
// L"Text Files (*.txt)|*.txt|All Files (*.*)|*.*"
[[nodiscard]] inline std::optional<std::vector<std::wstring>>
	open_files(const i_window* parent, std::wstring_view filterWithPipes)
{
	OPENFILENAME         ofn{};
	std::vector<wchar_t> multiBuf(65536, L'\0'); // http://www.askjf.com/?q=2179s http://www.askjf.com/?q=2181s
	std::vector<wchar_t> multiZFilter = _wli::format_file_filter(filterWithPipes);

	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner   = parent->hwnd();
	ofn.lpstrFilter = &multiZFilter[0];
	ofn.lpstrFile   = &multiBuf[0];
	ofn.nMaxFile    = static_cast<DWORD>(multiBuf.size()); // including terminating null
	ofn.Flags       = OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT | OFN_EXPLORER | OFN_ENABLESIZING;
	//ofn.FlagsEx = OFN_EX_NOPLACESBAR;
	// Call to GetOpenFileName() causes "First-chance exception (KernelBase.dll): The RPC server is unavailable."
	// in debug mode, but nothing else happens. The only way to get rid of it was using OFN_EX_NOPLACESBAR flag,
	// don't know why.

	if (!GetOpenFileNameW(&ofn)) {
		DWORD err = CommDlgExtendedError();
		if (err == FNERR_BUFFERTOOSMALL) {
			throw std::runtime_error(
				"GetOpenFileName failed, buffer is too small");
		}
		return {};
	}

	std::vector<std::wstring> strs = str::split_multi_zero(&multiBuf[0]);
	if (strs.empty()) {
		throw std::runtime_error("GetOpenFileName didn't return multiple strings.");
	}

	std::vector<std::wstring> files;

	if (strs.size() == 1) { // if user selected only 1 file, the string is the full path, and that's all
		files.emplace_back(strs[0]);
	} else { // user selected 2 or more files
		std::wstring& basePath = strs[0]; // 1st string is the base path; others are the filenames
		files.resize(strs.size() - 1); // alloc return buffer

		for (size_t i = 0; i < strs.size() - 1; ++i) {
			files[i].reserve(basePath.length() + strs[i + 1].size() + 1); // room for backslash
			files[i] = basePath;
			files[i].append(L"\\").append(strs[i + 1]); // concat folder + file
		}
		std::sort(files.begin(), files.end());
	}
	return files;
}

// File save dialog.
// Uses C# filter syntax:
// L"Text Files (*.txt)|*.txt|All Files (*.*)|*.*"
[[nodiscard]] inline std::optional<std::wstring>
	save_file(const i_window* parent, std::wstring_view filterWithPipes,
		std::wstring_view defFile, std::wstring_view defExtension)
{
	OPENFILENAME         ofn{};
	wchar_t              tmpBuf[MAX_PATH]{};
	std::vector<wchar_t> multiZFilter = _wli::format_file_filter(filterWithPipes);

	if (!defFile.empty()) lstrcpyW(tmpBuf, defFile.data());

	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner   = parent->hwnd();
	ofn.lpstrFilter = &multiZFilter[0];
	ofn.lpstrFile   = tmpBuf;
	ofn.nMaxFile    = ARRAYSIZE(tmpBuf);
	ofn.Flags       = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
	// If absent, no default extension is appended.
	// If present, even if empty, default extension is appended; if no default
	// extension, first one of the filter is appended.
	ofn.lpstrDefExt = defExtension.data();

	if (GetSaveFileNameW(&ofn)) {
		return tmpBuf;
	}
	return {};
}

// Folder choose dialog.
// Returns path without trailing backslash.
[[nodiscard]] inline std::optional<std::wstring> choose_folder(const i_window* parent)
{
	com::lib comLib{com::lib::init::NOW};
	//LPITEMIDLIST pidlRoot = 0;
	//if (defFolder) SHParseDisplayName(defFolder, nullptr, &pidlRoot, 0, nullptr);

	BROWSEINFOW bi{};
	bi.hwndOwner = parent->hwnd();
	bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;

	PIDLIST_ABSOLUTE pidl = SHBrowseForFolderW(&bi);
	if (!pidl) return {}; // user cancelled

	wchar_t tmpbuf[MAX_PATH]{};
	if (!SHGetPathFromIDListW(pidl, tmpbuf)) {
		throw std::runtime_error("SHGetPathFromIDList failed.");
	}
	return tmpbuf;
}

}//namespace wl::sys_dlg