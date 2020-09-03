/**
 * Part of WinLamb - Win32 API Lambda Library
 * https://github.com/rodrigocfd/winlamb
 * This library is released under the MIT License
 */

#pragma once
#include <array>
#include <optional>
#include <string_view>
#include <system_error>
#include <vector>
#include <Windows.h>
#include "com.h"
#include "str.h"
#pragma comment(lib, "Version.lib")

// Executable-related utilities.
namespace wl::exe {

// Retrieves the program's command line, with the tokens parsed.
[[nodiscard]] inline std::vector<std::wstring> command_line()
{
	return str::split_quoted(GetCommandLineW());
}

// Retrieves path to current running EXE itself.
[[nodiscard]] inline std::wstring own_path()
{
	wchar_t buf[MAX_PATH + 1]{};
	if (!GetModuleFileNameW(nullptr, buf, ARRAYSIZE(buf))) { // full path name
		throw std::system_error(GetLastError(), std::system_category(),
			"GetModuleFileName failed.");
	}
	std::wstring ret = buf;
	ret.resize(ret.find_last_of(L'\\')); // truncate removing EXE filename and trailing backslash

#if defined(_MSC_VER) && defined(_DEBUG)
	ret.resize(ret.find_last_of(L'\\')); // bypass "Debug" folder, remove trailing backslash too
#ifdef _WIN64
	ret.resize(ret.find_last_of(L'\\')); // bypass "x64" folder, remove trailing backslash again
#endif
#endif

	return ret;
}

// Reads the version information from an EXE or DLL.
// If exeOrDllPath is not specified, reads from current running EXE itself.
[[nodiscard]] inline std::optional<std::array<UINT, 4>>
	read_version(std::wstring_view exeOrDllPath)
{
	DWORD szVer = GetFileVersionInfoSizeW(exeOrDllPath.data(), nullptr);
	if (!szVer) {
		throw std::system_error(GetLastError(), std::system_category(),
			"GetFileVersionInfoSize failed.");
	}

	std::vector<wchar_t> infoBlock(szVer, L'\0');
	if (!GetFileVersionInfoW(exeOrDllPath.data(), 0, szVer, &infoBlock[0])) {
		throw std::system_error(GetLastError(), std::system_category(),
			"GetFileVersionInfo failed.");
	}

	BYTE* lpBuf = nullptr;
	UINT blockSize = 0;
	if (!VerQueryValueW(&infoBlock[0], L"\\",
		reinterpret_cast<void**>(&lpBuf), &blockSize) ||
		!blockSize)
	{
		return {}; // no information available, not an error
	}

	VS_FIXEDFILEINFO* verInfo = reinterpret_cast<VS_FIXEDFILEINFO*>(lpBuf);
	return {{
		(verInfo->dwFileVersionMS >> 16) & 0xffff,
		(verInfo->dwFileVersionMS >>  0) & 0xffff,
		(verInfo->dwFileVersionLS >> 16) & 0xffff,
		(verInfo->dwFileVersionLS >>  0) & 0xffff
	}};
}

// Reads the version information from an EXE or DLL.
// If exeOrDllPath is not specified, reads from current running EXE itself.
[[nodiscard]] inline std::optional<std::array<UINT, 4>> read_version()
{
	wchar_t buf[MAX_PATH + 1]{};
	if (!GetModuleFileNameW(nullptr, buf, ARRAYSIZE(buf))) { // full path name
		throw std::system_error(GetLastError(), std::system_category(),
			"GetModuleFileName failed.");
	}
	return read_version(buf);
}

// Synchronous execution of a command line, in a separated process.
inline DWORD run(std::wstring_view cmdLine)
{
	SECURITY_ATTRIBUTES sa{};
	sa.nLength = sizeof(sa);
	sa.bInheritHandle = TRUE;

	STARTUPINFO si{};
	si.cb = sizeof(si);
	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_SHOW;

	PROCESS_INFORMATION pi{};
	DWORD dwExitCode = 1; // returned by executed program

	std::wstring cmdLine2 = cmdLine.data(); // http://blogs.msdn.com/b/oldnewthing/archive/2009/06/01/9673254.aspx

	if (!CreateProcessW(nullptr, &cmdLine2[0], &sa, nullptr, FALSE,
		0, nullptr, nullptr, &si, &pi))
	{
		throw std::system_error(GetLastError(), std::system_category(),
			"CreateProcess failed.");
	}

	WaitForSingleObject(pi.hProcess, INFINITE); // the program flow is stopped here to wait
	GetExitCodeProcess(pi.hProcess, &dwExitCode);
	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);
	return dwExitCode;
}

// Runs the shell-associated program to the given file
// Example: for a TXT file, will run Notepad.
inline void run_associated_shell(std::wstring_view file, INT showCmd = SW_SHOWNORMAL)
{
	com::lib comLib{com::lib::init::NOW};

	int h = static_cast<int>(
		reinterpret_cast<INT_PTR>(
			ShellExecuteW(nullptr, L"open", file.data(), nullptr, nullptr, showCmd) ));

	if (h <= 8) {
		throw std::system_error(h, std::system_category(),
			"ShellExecute failed.");
	} else if (h <= 32) {
		throw std::runtime_error(
			str::unicode_to_ansi(
				str::format(L"ShellExecute failed: error %d.", h) ));
	}
}

}//namespace wl::exe