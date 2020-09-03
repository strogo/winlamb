/**
 * Part of WinLamb - Win32 API Lambda Library
 * https://github.com/rodrigocfd/winlamb
 * This library is released under the MIT License
 */

#pragma once
#include <functional>
#include <stdexcept>
#include <string_view>
#include <vector>
#include <Windows.h>
#include <winhttp.h>
#include "internals/download_session.h"
#include "internals/download_url.h"
#include "insert_order_map.h"
#include "str.h"
#pragma comment(lib, "Winhttp.lib")

namespace wl {

// Manages internet download operations with WinHTTP.
class download final {
private:
	_wli::download_session _session;
	HINTERNET _hConnect = nullptr, _hRequest = nullptr;
	std::wstring _url, _verb, _referrer;
	size_t _contentLength = 0, _totalReceived = 0;
	insert_order_map<std::wstring, std::wstring> _requestHeaders;
	insert_order_map<std::wstring, std::wstring> _responseHeaders;
	std::function<void()> _startCallback, _progressCallback;

public:
	// Stores the downloaded data.
	std::vector<BYTE> data;

	~download() { this->abort(); }

	download(std::wstring_view url, std::wstring_view verb = L"GET") :
		_url{url}, _verb{verb} { }

	[[nodiscard]] const _wli::download_session& session() noexcept { return this->_session; }
	[[nodiscard]] size_t content_length() const noexcept { return this->_contentLength; }
	[[nodiscard]] size_t total_received() const noexcept { return this->_totalReceived; }
	[[nodiscard]] const insert_order_map<std::wstring, std::wstring>& request_headers() const noexcept { return this->_requestHeaders; }
	[[nodiscard]] const insert_order_map<std::wstring, std::wstring>& response_headers() const noexcept { return this->_responseHeaders; }

	// Releases all handles, session and data buffer are kept.
	download& abort() noexcept
	{
		if (this->_hRequest) {
			WinHttpCloseHandle(this->_hRequest);
			this->_hRequest = nullptr;
		}
		if (this->_hConnect) {
			WinHttpCloseHandle(this->_hConnect);
			this->_hConnect = nullptr;
		}
		this->_contentLength = this->_totalReceived = 0;
		return *this;
	}

	// Adds a new request header entry.
	download& add_request_header(std::wstring_view name, std::wstring_view value)
	{
		this->_requestHeaders.emplace(name.data(), value.data());
		return *this;
	}

	// Defines a lambda to be called once, right after the download starts.
	// func: []() { }
	template<typename F>
	download& on_start(F&& func) noexcept
	{
		this->_startCallback = std::move(func);
		return *this;
	}

	// Defines a lambda do be called each time a chunk of bytes is received.
	// func: []() { }
	template<typename F>
	download& on_progress(F&& func) noexcept
	{
		this->_progressCallback = std::move(func);
		return *this;
	}

	// Sets the referrer of the request.
	download& set_referrer(std::wstring_view referrer)
	{
		this->_referrer = referrer;
		return *this;
	}

	// Effectively starts the download, returning only after it completes.
	download& start()
	{
		if (this->_hConnect) {
			throw std::logic_error("A download is already in progress.");
		} else if (this->_url.empty()) {
			throw std::invalid_argument("Blank URL.");
		}

		if (!this->_session.hsession()) {
			this->_session.open();
		}

		this->_contentLength = this->_totalReceived = 0;
		this->_init_handles();
		this->_contact_server();
		this->_parse_headers();
		this->data.clear(); // prepare buffer to receive data
		if (this->_contentLength) { // server informed content length?
			this->data.reserve(this->_contentLength);
		}

		if (this->_startCallback) this->_startCallback(); // run user callback

		if (this->_hConnect && this->_hRequest) { // user didn't call abort()
			for (;;) {
				DWORD incomingBytes = this->_get_incoming_byte_count(); // chunk size about to come
				if (!incomingBytes) break; // no more bytes remaining
				this->_receive_bytes(incomingBytes); // chunk will be appended into this->data
				if (this->_progressCallback) this->_progressCallback();
				if (!this->_hConnect && !this->_hRequest) break; // user called abort()
			}
		}

		return this->abort(); // cleanup
	}

private:
	void _init_handles()
	{
		// Crack the URL.
		_wli::download_url crackedUrl{this->_url};

		// Open the connection handle.
		this->_hConnect = this->_session.connect(crackedUrl.host().data(), crackedUrl.port());

		// Build the request handle.
		std::wstring fullPath = crackedUrl.path_and_extra();
		this->_hRequest = WinHttpOpenRequest(this->_hConnect, this->_verb.c_str(),
			fullPath.c_str(), nullptr,
			this->_referrer.empty() ? WINHTTP_NO_REFERER : this->_referrer.c_str(),
			WINHTTP_DEFAULT_ACCEPT_TYPES,
			crackedUrl.scheme() == INTERNET_SCHEME_HTTPS ? WINHTTP_FLAG_SECURE : 0);
		if (!this->_hRequest) {
			throw std::system_error(GetLastError(), std::system_category(),
				"WinHttpOpenRequest failed.");
		}
	}

	void _contact_server()
	{
		// Add the request headers to request handle.
		std::wstring rhTmp;
		rhTmp.reserve(20);
		for (const insert_order_map<std::wstring, std::wstring>::entry& rh : this->_requestHeaders) {
			rhTmp = rh.key;
			rhTmp += L": ";
			rhTmp += rh.val;

			if (!WinHttpAddRequestHeaders(this->_hRequest, rhTmp.c_str(),
				static_cast<ULONG>(-1L), WINHTTP_ADDREQ_FLAG_ADD))
			{
				throw std::system_error(GetLastError(), std::system_category(),
					"WinHttpAddRequestHeaders failed");
			}
		}

		// Send the request to server.
		if (!WinHttpSendRequest(this->_hRequest, WINHTTP_NO_ADDITIONAL_HEADERS,
			0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0))
		{
			throw std::system_error(GetLastError(), std::system_category(),
				"WinHttpSendRequest failed");
		}

		// Receive the response from server.
		if (!WinHttpReceiveResponse(this->_hRequest, nullptr)) {
			throw std::system_error(GetLastError(), std::system_category(),
				"WinHttpReceiveResponse failed");
		}
	}

	void _parse_headers()
	{
		// Retrieve the response header.
		DWORD rehSize = 0;
		WinHttpQueryHeaders(this->_hRequest, WINHTTP_QUERY_RAW_HEADERS_CRLF,
			WINHTTP_HEADER_NAME_BY_INDEX, WINHTTP_NO_OUTPUT_BUFFER, &rehSize, WINHTTP_NO_HEADER_INDEX);

		std::wstring rawReh(rehSize / sizeof(wchar_t), L'\0'); // raw response headers

		if (!WinHttpQueryHeaders(this->_hRequest, WINHTTP_QUERY_RAW_HEADERS_CRLF,
			WINHTTP_HEADER_NAME_BY_INDEX, &rawReh[0], &rehSize, WINHTTP_NO_HEADER_INDEX))
		{
			throw std::system_error(GetLastError(), std::system_category(),
				"WinHttpQueryHeaders failed");
		}

		// Parse the raw response headers into an associative array.
		this->_responseHeaders.clear();
		std::vector<std::wstring> lines = str::split_lines(rawReh);

		for (const std::wstring& line : lines) {
			if (line.empty()) {
				continue;
			}
			size_t colonIdx = line.find_first_of(L':');
			if (colonIdx == std::wstring::npos) { // not a key/value pair, probably response line
				this->_responseHeaders[L""] = line; // empty key
			} else {
				std::wstring kk = line.substr(0, colonIdx);
				std::wstring vv = line.substr(colonIdx + 1, line.length() - (colonIdx + 1));
				this->_responseHeaders[str::trim(kk)] = str::trim(vv);
			}
		}

		// Retrieve content length, if informed by server.
		auto cl = this->_responseHeaders.find(L"Content-Length");
		if (cl.has_value() && str::is_uint(cl.value().get())) { // server informed content length?
			this->_contentLength = std::stoul(cl.value().get());
		}
	}

	[[nodiscard]] DWORD _get_incoming_byte_count()
	{
		DWORD count = 0;
		if (!WinHttpQueryDataAvailable(this->_hRequest, &count)) { // how many bytes are about to come
			throw std::system_error(GetLastError(), std::system_category(),
				"WinHttpQueryDataAvailable failed");
		}
		return count;
	}

	void _receive_bytes(DWORD nBytesToRead)
	{
		DWORD readCountDummy = 0;
		this->data.resize(this->data.size() + nBytesToRead); // make room

		if (!WinHttpReadData(this->_hRequest,
			&this->data[this->data.size() - nBytesToRead], // append to buffer
			nBytesToRead, &readCountDummy) )
		{
			throw std::system_error(GetLastError(), std::system_category(),
				"WinHttpReadData failed");
		}

		this->_totalReceived += nBytesToRead; // update total downloaded count
	}
};

}//namespace wl