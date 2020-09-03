/**
 * Part of WinLamb - Win32 API Lambda Library
 * https://github.com/rodrigocfd/winlamb
 * This library is released under the MIT License
 */

#pragma once
#include <system_error>
#include <crtdbg.h>
#include <Windows.h>
#include <CommCtrl.h>
#include "../str.h"
#pragma comment(lib, "Comctl32.lib")

namespace _wli {

enum class post_quit_on_catch { YES, NO };

// Executes a lambda and catches any exception, showing a MessageBox().
template<typename F>
inline void catch_all_excps(F&& func, post_quit_on_catch catchAction) noexcept
{
	// Intended to handle the exceptions of user lambdas.
	// https://stackoverflow.com/a/18792495
	try {

		try {
			func();
		} catch (const std::system_error& e) {
			MessageBoxW(nullptr,
				wl::str::format(L"[%d 0x%02x] %s",
					e.code().value(),
					e.code().value(),
					wl::str::ansi_to_unicode(e.what())
				).c_str(),
				L"System exception", MB_ICONERROR);
			throw;
		} catch (const std::runtime_error& e) {
			MessageBoxA(nullptr, e.what(), "Runtime exception", MB_ICONERROR);
			throw;
		} catch (const std::invalid_argument& e) {
			MessageBoxA(nullptr, e.what(), "Invalid argument exception", MB_ICONERROR);
			throw;
		} catch (const std::logic_error& e) {
			MessageBoxA(nullptr, e.what(), "Logic exception", MB_ICONERROR);
			throw;
		} catch (const std::exception& e) {
			MessageBoxA(nullptr, e.what(), "Exception", MB_ICONERROR);
			throw;
		} catch (...) {
			MessageBoxW(nullptr, L"An unknown exception was thrown.",
				L"Unknown exception", MB_ICONERROR);
			throw;
		}

	} catch (...) {
		if (catchAction == post_quit_on_catch::YES) {
			PostQuitMessage(-1);
		}
	}
}

// Instantiates a class and calls run_as_main(), catching any exception.
template<typename wnd_mainT>
int catch_run_main(HINSTANCE hInst, int cmdShow) noexcept
{
	struct LeakDetect {
		~LeakDetect() { _ASSERT(!_CrtDumpMemoryLeaks()); }
	};
	static LeakDetect leakDetect; // destructor will run after all static destructors

	int ret = 0;
	catch_all_excps([&ret, hInst, cmdShow]() {
		wnd_mainT w; // instantiate main window object
		ret = w.run_as_main(hInst, cmdShow);
	}, post_quit_on_catch::NO);
	return ret;
}

}//namespace _wli

// Instantiates a main class into a generic WinMain function.
#define RUN(wnd_mainT) \
int wWinMain(_In_ HINSTANCE hInst, _In_opt_ HINSTANCE, _In_ LPWSTR, _In_ int cmdShow) { \
	return _wli::catch_run_main<wnd_mainT>(hInst, cmdShow); \
}