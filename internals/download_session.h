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
#include <winhttp.h>
#pragma comment(lib, "Winhttp.lib")

namespace _wli {

// Manages an HINTERNET resource.
class download_session final {
private:
	HINTERNET _hSession = nullptr;

public:
	~download_session() { this->close(); }

	download_session() = default;
	download_session(download_session&& other) noexcept { this->operator=(std::move(other)); } // movable only

	bool operator==(const download_session& other) const noexcept { return this->_hSession == other._hSession; }
	bool operator!=(const download_session& other) const noexcept { return !this->operator==(other); }

	download_session& operator=(download_session&& other) noexcept
	{
		this->close();
		std::swap(this->_hSession, other._hSession);
		return *this;
	}

	// Returns the HINTERNET.
	[[nodiscard]] HINTERNET hsession() const noexcept { return this->_hSession; }

	// Calls WinHttpCloseHandle().
	void close() noexcept
	{
		if (this->_hSession) {
			WinHttpCloseHandle(this->_hSession);
			this->_hSession = nullptr;
		}
	}

	// Calls WinHttpConnect().
	[[nodiscard]] HINTERNET connect(const wchar_t* pswzServerName, INTERNET_PORT nServerPort) const
	{
		HINTERNET hConnect = WinHttpConnect(this->_hSession, pswzServerName, nServerPort, 0);
		if (!hConnect) {
			throw std::system_error(GetLastError(), std::system_category(),
				"WinHttpConnect failed.");
		}
		return hConnect;
	}

	// Calls WinHttpCheckPlatform() and WinHttpOpen().
	download_session& open(std::wstring_view userAgent = L"WinLamb/1.0")
	{
		// If session is already open, do nothing and let it pass.
		if (!this->_hSession) {
			// http://social.msdn.microsoft.com/forums/en-US/vclanguage/thread/45ccd91c-6794-4f9b-8f4f-865c76cc146d
			if (!WinHttpCheckPlatform()) {
				throw std::system_error(GetLastError(), std::system_category(),
					"WinHttpCheckPlatform failed, this platform is not supported by WinHTTP.");
			}

			this->_hSession = WinHttpOpen(userAgent.data(), WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
				WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
			if (!this->_hSession) {
				throw std::system_error(GetLastError(), std::system_category(),
					"WinHttpOpen failed.");
			}
		}

		return *this;
	}
};

}//namespace _wli