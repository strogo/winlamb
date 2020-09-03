/**
 * Part of WinLamb - Win32 API Lambda Library
 * https://github.com/rodrigocfd/winlamb
 * This library is released under the MIT License
 */

#pragma once
#include <system_error>
#include <utility>
#include <Windows.h>
#include <objbase.h>

namespace wl::com {

// Manages a COM pointer.
template<typename T>
class ptr final {
private:
	T* _ptr = nullptr;

public:
	~ptr() { this->release(); }
	ptr() = default;
	ptr(ptr&& other) noexcept { this->operator=(std::move(other)); }

	[[nodiscard]] const T* operator->() const noexcept { return this->_ptr; } // to call COM methods through ->
	[[nodiscard]] T*       operator->() noexcept       { return this->_ptr; }

	// Tells if the underlying pointer is null.
	[[nodiscard]] bool empty() const noexcept { return this->_ptr == nullptr; }

	// Returns the raw pointer to the COM interface pointer.
	[[nodiscard]] T** raw_pptr() noexcept { return &this->_ptr; }

	ptr& operator=(ptr&& other) noexcept
	{
		this->release();
		std::swap(this->_ptr, other._ptr);
		return *this;
	}

	// Returns a safe clone of the COM pointer.
	[[nodiscard]] ptr clone() const noexcept
	{
		this->_ptr->AddRef();

		ptr clonedObj{};
		clonedObj._ptr = this->_ptr;
		return clonedObj;
	}

	// Calls IUnknown::QueryInterface() with the given REFIID.
	template<typename Q>
	[[nodiscard]] ptr<Q> query_interface(REFIID iid_any)
	{
		ptr<Q> ptrBuf;
		HRESULT hr = this->_ptr->QueryInterface(iid_any,
			reinterpret_cast<void**>(ptrBuf.raw_pptr()));
		if (FAILED(hr)) {
			throw std::system_error(hr, std::system_category(),
				"QueryInterface failed.");
		}
		return ptrBuf;
	}

	// Calls IUnknown::QueryInterface() with IID_PPV_ARGS().
	template<typename Q>
	[[nodiscard]] ptr<Q> query_interface()
	{
		ptr<Q> ptrBuf;
		HRESULT hr = this->_ptr->QueryInterface(IID_PPV_ARGS(ptrBuf.raw_pptr()));
		if (FAILED(hr)) {
			throw std::system_error(hr, std::system_category(),
				"QueryInterface failed.");
		}
		return ptrBuf;
	}

	// Calls IUnknown::Release().
	void release() noexcept
	{
		if (this->_ptr) {
			this->_ptr->Release();
			this->_ptr = nullptr;
		}
	}
};

// Creates a COM object by calling CoCreateInstance() with the given REFIID.
template<typename T>
[[nodiscard]] inline ptr<T> co_create_instance(
	REFCLSID clsid_any, REFIID iid_any, DWORD clsctxContext = CLSCTX_INPROC_SERVER)
{
	ptr<T> ptrBuf;
	HRESULT hr = CoCreateInstance(clsid_any, nullptr,
		clsctxContext, iid_any, reinterpret_cast<LPVOID*>(ptrBuf.raw_pptr()));
	if (FAILED(hr)) {
		throw std::system_error(hr, std::system_category(),
			"CoCreateInstance failed.");
	}
	return ptrBuf;
}

// Creates a COM object by calling CoCreateInstance() with IID_PPV_ARGS().
template<typename T>
[[nodiscard]] inline ptr<T> co_create_instance(
	REFCLSID clsid_any, DWORD clsctxContext = CLSCTX_INPROC_SERVER)
{
	ptr<T> ptrBuf;
	HRESULT hr = CoCreateInstance(clsid_any, nullptr,
		clsctxContext, IID_PPV_ARGS(ptrBuf.raw_pptr()));
	if (FAILED(hr)) {
		throw std::system_error(hr, std::system_category(),
			"CoCreateInstance failed.");
	}
	return ptrBuf;
}

}//namespace wl::com