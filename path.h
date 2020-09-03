/**
 * Part of WinLamb - Win32 API Lambda Library
 * https://github.com/rodrigocfd/winlamb
 * This library is released under the MIT License
 */

#pragma once
#include <stdexcept>
#include <string_view>
#include <system_error>
#include <vector>
#include <Windows.h>
#include <shellapi.h>
#include "internals/path_aux.h"
#include "str.h"

// Utilities to file path operations.
namespace wl::path {

// Forward declarations.
bool is_dir(std::wstring_view) noexcept;

// In-place changes the extension to the given one, or appends if it has no extension.
inline std::wstring& change_extension(std::wstring& filePath,
	std::wstring_view newExtension)
{
	size_t dotIdx = filePath.find_last_of(L'.');
	if (dotIdx != std::wstring::npos) { // filePath already has an extension
		filePath.resize(dotIdx + 1); // truncate after the dot
	} else { // filePath doesn't have an extension
		filePath.append(1, L'.');
	}
	filePath.append(newExtension[0] == L'.' ? &newExtension[1] : newExtension);
	return filePath;
}

// Creates a new directory.
inline void create_dir(std::wstring_view dirPath)
{
	if (!CreateDirectoryW(dirPath.data(), nullptr)) {
		throw std::system_error(GetLastError(), std::system_category(),
			"CreateDirectory failed");
	}
}

// Deletes a file, or a directory recursively.
inline void del(std::wstring_view fileOrFolder)
{
	if (is_dir(fileOrFolder)) {
		// http://stackoverflow.com/q/1468774/6923555
		wchar_t szDir[MAX_PATH + 1]{}; // +1 for the double null terminate
		lstrcpyW(szDir, fileOrFolder.data());

		SHFILEOPSTRUCTW fos{};
		fos.wFunc = FO_DELETE;
		fos.pFrom = szDir;
		fos.fFlags = FOF_NO_UI;

		if (SHFileOperationW(&fos)) {
			throw std::runtime_error(
				"SHFileOperation failed to recursively delete directory, unspecified error.");
		}
	} else {
		if (!DeleteFileW(fileOrFolder.data())) {
			throw std::system_error(GetLastError(), std::system_category(),
				"DeleteFile failed.");
		}
	}
}

// Tells if the given file path exists.
[[nodiscard]] inline bool exists(std::wstring_view filePath) noexcept
{
	return GetFileAttributesW(filePath.data()) != INVALID_FILE_ATTRIBUTES;
}

// Retrieves the file name form a file path.
[[nodiscard]] inline std::wstring file_from(std::wstring_view filePath)
{
	std::wstring ret = filePath.data();
	size_t found = ret.find_last_of(L'\\');
	if (found != std::wstring::npos) {
		ret.erase(0, found + 1);
	}
	return ret;
}

// Retrieves the complete folder path from a file path.
// Won't include trailing backslash.
[[nodiscard]] inline std::wstring folder_from(std::wstring_view filePath)
{
	std::wstring ret = filePath.data();
	size_t found = ret.find_last_of(L'\\');
	if (found != std::wstring::npos) {
		ret.resize(found);
	}
	return ret;
}

// Tells if the file has the given extension, case-insensitive.
[[nodiscard]] inline bool has_extension(
	std::wstring_view filePath, std::wstring_view extension) noexcept
{
	if (extension[0] == L'.') { // extension starts with dot, compare right away
		return str::ends_withi(filePath, extension);
	}

	wchar_t dotExtension[32] = L"."; // arbitrary buffer length
	lstrcatW(dotExtension, extension.data());
	return str::ends_withi(filePath, dotExtension);
}

// Tells if the file has the given extension, case-insensitive.
[[nodiscard]] inline bool has_extension(
	std::wstring_view filePath, std::initializer_list<std::wstring_view> extensions) noexcept
{
	for (std::wstring_view ex : extensions) {
		if (has_extension(filePath, ex)) {
			return true;
		}
	}
	return false;
}

// Tells if a path is a directory.
[[nodiscard]] inline bool is_dir(std::wstring_view anyPath) noexcept
{
	return (GetFileAttributesW(anyPath.data()) & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

// Tells if the path is hidden.
[[nodiscard]] inline bool is_hidden(std::wstring_view anyPath) noexcept
{
	return (GetFileAttributesW(anyPath.data()) & FILE_ATTRIBUTE_HIDDEN) != 0;
}

// Tells if two paths are the same.
// Simple case-insensitive string comparison.
[[nodiscard]] inline bool is_same(std::wstring_view path1, std::wstring_view path2) noexcept
{
	return str::eqi(path1, path2);
}

// Lists files within a directory according to a pattern,
// like "C:\\files\\*.txt". Just "*" will bring all.
[[nodiscard]] inline std::vector<std::wstring> list_dir(std::wstring_view pathAndPattern)
{
	std::vector<std::wstring> files;

	WIN32_FIND_DATA wfd{};
	HANDLE hFind = FindFirstFileW(pathAndPattern.data(), &wfd);
	if (hFind == INVALID_HANDLE_VALUE) {
		DWORD err = GetLastError();
		if (err == ERROR_FILE_NOT_FOUND) {
			return files;
		} else {
			throw std::system_error(err, std::system_category(),
				"FindFirstFile failed.");
		}
	}

	std::wstring pathPat = pathAndPattern
		.substr(0, pathAndPattern.find_last_of(L'\\')) // no trailing backslash
		.data();

	do {
		if (*wfd.cFileName
			&& lstrcmpiW(wfd.cFileName, L".") // do not add current and parent paths
			&& lstrcmpiW(wfd.cFileName, L".."))
		{
			files.emplace_back(pathPat);
			files.back().append(L"\\").append(wfd.cFileName);
		}
	} while (FindNextFileW(hFind, &wfd));

	FindClose(hFind);
	return files;
}

// System path locations that can be retrieved with sys_path().
enum class to { MY_DOCUMENTS, MY_MUSIC, MY_PICTURES, MY_VIDEO,
	DESKTOP, APP_DATA, LOCAL_APP_DATA, COMMON_APP_DATA,
	PROGRAM_FILES, PROGRAM_FILES_X86, TEMP };

// Retrieves a system path.
// Returned string won't have a trailing backslash.
[[nodiscard]] inline std::wstring sys_path(to pathToRetrieve)
{
	switch (pathToRetrieve) {
	case to::MY_DOCUMENTS:      return _wli::sys_path_shell(CSIDL_MYDOCUMENTS);
	case to::MY_MUSIC:          return _wli::sys_path_shell(CSIDL_MYMUSIC);
	case to::MY_PICTURES:       return _wli::sys_path_shell(CSIDL_MYPICTURES);
	case to::MY_VIDEO:          return _wli::sys_path_shell(CSIDL_MYVIDEO);
	case to::DESKTOP:           return _wli::sys_path_shell(CSIDL_DESKTOPDIRECTORY);
	case to::APP_DATA:          return _wli::sys_path_shell(CSIDL_APPDATA);
	case to::LOCAL_APP_DATA:    return _wli::sys_path_shell(CSIDL_LOCAL_APPDATA);
	case to::COMMON_APP_DATA:   return _wli::sys_path_shell(CSIDL_COMMON_APPDATA);
	case to::PROGRAM_FILES:     return _wli::sys_path_shell(CSIDL_PROGRAM_FILES);
	case to::PROGRAM_FILES_X86: return _wli::sys_path_shell(CSIDL_PROGRAM_FILESX86);
	case to::TEMP:              return _wli::sys_path_temp();
	}
	return {}; // never reached
}

// In-place removes the trailing backslash, if any.
inline std::wstring& trim_backslash(std::wstring& filePath)
{
	while (filePath.back() == L'\\') {
		filePath.resize(filePath.length() - 1);
	}
	return filePath;
}

}//namespace wl::path