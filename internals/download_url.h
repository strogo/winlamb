/**
 * Part of WinLamb - Win32 API Lambda Library
 * https://github.com/rodrigocfd/winlamb
 * This library is released under the MIT License
 */

#pragma once
#include <string_view>
#include <system_error>
#include <Windows.h>
#include <winhttp.h>
#pragma comment(lib, "Winhttp.lib")

namespace _wli {

// Calls WinHttpCrackUrl() and manages the URL_COMPONENTS result.
class download_url final {
private:
	wchar_t _schemeName[16]{}, _host[64]{}, _user[64]{},
		_password[64]{}, _path[256]{}, _extra[256]{};
	INTERNET_PORT _port = 0;
	INTERNET_SCHEME _scheme = 0;

public:
	download_url(std::wstring_view address)
	{
		URL_COMPONENTS uc{};
		uc.dwStructSize = sizeof(URL_COMPONENTS);

		uc.lpszScheme = this->_schemeName;
		uc.dwSchemeLength = ARRAYSIZE(this->_schemeName);

		uc.lpszHostName = this->_host;
		uc.dwHostNameLength = ARRAYSIZE(this->_host);

		uc.lpszUserName = this->_user;
		uc.dwUserNameLength = ARRAYSIZE(this->_user);

		uc.lpszPassword = this->_password;
		uc.dwPasswordLength = ARRAYSIZE(this->_password);

		uc.lpszUrlPath = this->_path;
		uc.dwUrlPathLength = ARRAYSIZE(this->_path);

		uc.lpszExtraInfo = this->_extra;
		uc.dwExtraInfoLength = ARRAYSIZE(this->_extra);

		if (!WinHttpCrackUrl(address.data(), 0, 0, &uc)) {
			throw std::system_error(GetLastError(), std::system_category(),
				"WinHttpCrackUrl failed.");
		}

		this->_port = uc.nPort;
		this->_scheme = uc.nScheme;
	}

	download_url(download_url&) = default;
	download_url& operator=(download_url&&) = default; // movable only

	[[nodiscard]] std::wstring_view scheme_name() const noexcept { return this->_schemeName; }
	[[nodiscard]] std::wstring_view host() const noexcept        { return this->_host; }
	[[nodiscard]] std::wstring_view user() const noexcept        { return this->_user; }
	[[nodiscard]] std::wstring_view password() const noexcept    { return this->_password; }
	[[nodiscard]] std::wstring_view path() const noexcept        { return this->_path; }
	[[nodiscard]] std::wstring_view extra() const noexcept       { return this->_extra; }
	[[nodiscard]] INTERNET_PORT     port() const noexcept        { return this->_port; }
	[[nodiscard]] INTERNET_SCHEME   scheme() const noexcept      { return this->_scheme; }

	[[nodiscard]] std::wstring path_and_extra() const
	{
		std::wstring ret = this->_path;
		ret.append(this->_extra);
		return ret;
	}
};

}//namespace _wli