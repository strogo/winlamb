/**
 * Part of WinLamb - Win32 API Lambda Library
 * https://github.com/rodrigocfd/winlamb
 * This library is released under the MIT License
 */

#pragma once
#include <chrono>
#include <string_view>
#include <system_error>
#include <utility>
#include <Windows.h>
#include "bin.h"
#include "str.h"
#include "time.h"

namespace wl {

// Manages a low-level HANDLE to a file.
// Calls CloseHandle() on destructor.
class file final {
public:
	struct file_times final {
		std::chrono::system_clock::time_point
			creation, lastAccess, lastWrite;
	};

private:
	HANDLE _hFile = nullptr;

public:
	~file() { this->close(); }
	file() = default;
	file(file&& other) noexcept { this->operator=(std::move(other)); } // movable only

	bool operator==(const file& other) const noexcept { return this->_hFile == other._hFile; }
	bool operator!=(const file& other) const noexcept { return !this->operator==(other); }

	file& operator=(file&& other) noexcept
	{
		this->close();
		std::swap(this->_hFile, other._hFile);
		return *this;
	}

	// Returns the HFILE.
	[[nodiscard]] HANDLE hfile() const noexcept { return this->_hFile; }

	file& open_existing_read(std::wstring_view filePath)       { return this->_open(filePath, GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING); }
	file& open_existing_read_write(std::wstring_view filePath) { return this->_open(filePath, GENERIC_READ | GENERIC_WRITE, 0, OPEN_EXISTING); }
	file& open_or_create(std::wstring_view filePath)           { return this->_open(filePath, GENERIC_READ | GENERIC_WRITE, 0, OPEN_ALWAYS); }

	// Calls CloseHandle().
	void close() noexcept
	{
		if (this->_hFile) {
			CloseHandle(this->_hFile);
			this->_hFile = nullptr;
		}
	}

	// Retrieves file creation, last access, and last write times.
	// The times are in system local time.
	file_times times() const
	{
		FILETIME creation{}, lastAccess{}, lastWrite{};
		if (!GetFileTime(this->_hFile, &creation, &lastAccess, &lastWrite)) {
			throw std::system_error(GetLastError(), std::system_category(),
				"GetFileTime failed.");
		}
		return {
			time::utc_to_local(time::to_time_point(creation)),
			time::utc_to_local(time::to_time_point(lastAccess)),
			time::utc_to_local(time::to_time_point(lastWrite))
		};
	}

	// Reads all file contents at once.
	[[nodiscard]] std::vector<BYTE> read_all() const
	{
		std::vector<BYTE> buf(this->size(), 0); // alloc right away
		DWORD bytesRead = 0;

		if (!ReadFile(this->_hFile, &buf[0], static_cast<DWORD>(buf.size()),
			&bytesRead, nullptr))
		{
			throw std::system_error(GetLastError(), std::system_category(),
				"ReadFile failed.");
		}
		return buf;
	}

	// Reads all file contents at once, and parses as wstring.
	[[nodiscard]] std::wstring read_all_as_string() const
	{
		return bin::parse_str(this->read_all());
	}

	// Calls SetFilePointer to set internal pointer to begin of the file.
	const file& rewind() const
	{
		LARGE_INTEGER lnum{};

		if (SetFilePointerEx(this->_hFile, lnum, nullptr, FILE_BEGIN)
			== INVALID_SET_FILE_POINTER)
		{
			throw std::system_error(GetLastError(), std::system_category(),
				"SetFilePointerEx failed to rewind the file.");
		}
		return *this;
	}

	// Truncates or expands the file, according to the new size; zero will empty the file.
	const file& set_new_size(size_t numBytes) const
	{
		if (this->size() != numBytes) { // otherwise nothing to do
			return *this;
		}

		LARGE_INTEGER lnum{};
		lnum.QuadPart = numBytes;

		if (SetFilePointerEx(this->_hFile, lnum, nullptr, FILE_BEGIN)
			== INVALID_SET_FILE_POINTER)
		{
			throw std::system_error(GetLastError(), std::system_category(),
				"SetFilePointerEx failed when setting new file size.");
		}

		if (!SetEndOfFile(this->_hFile)) {
			throw std::system_error(GetLastError(), std::system_category(),
				"SetEndOfFile failed when setting new file size.");
		}

		lnum.QuadPart = 0;

		if (SetFilePointerEx(this->_hFile, lnum, nullptr, FILE_BEGIN) // rewind
			== INVALID_SET_FILE_POINTER)
		{
			throw std::system_error(GetLastError(), std::system_category(),
				"SetFilePointerEx failed to rewind the file pointer when setting new file size.");
		}

		return *this;
	}

	// Retrieves the file size with GetFileSizeEx().
	[[nodiscard]] size_t size() const
	{
		LARGE_INTEGER buf{};
		if (!GetFileSizeEx(this->_hFile, &buf)) {
			throw std::system_error(GetLastError(), std::system_category(),
				"GetFileSizeEx failed.");
		}
		return buf.QuadPart;
	}

	// Writes content to file, wrapper to WriteFile.
	// File boundary will be expanded if needed.
	// Internal file pointer will move forward.
	const file& write(const BYTE* pData, size_t sz) const
	{
		DWORD dwWritten = 0;
		if (!WriteFile(this->_hFile, pData, static_cast<DWORD>(sz), &dwWritten, nullptr)) {
			throw std::system_error(GetLastError(), std::system_category(),
				"WriteFile failed.");
		}
		return *this;
	}

	// Writes content to file, wrapper to WriteFile.
	// File boundary will be expanded if needed.
	// Internal file pointer will move forward.
	const file& write(const std::vector<BYTE>& data) const
	{
		return this->write(&data[0], data.size());
	}

private:
	file& _open(std::wstring_view filePath, DWORD desiredAccess, DWORD shareMode,
		DWORD creationDisposition)
	{
		this->close();

		this->_hFile = CreateFileW(filePath.data(), desiredAccess, shareMode,
			nullptr, creationDisposition, FILE_ATTRIBUTE_NORMAL, nullptr);

		if (this->_hFile == INVALID_HANDLE_VALUE) {
			this->_hFile = nullptr;
			throw std::runtime_error(
				str::unicode_to_ansi(
					str::format(L"CreateFile failed \"%s\".", filePath) ));
		}
		return *this;
	}
};

}//namespace wl