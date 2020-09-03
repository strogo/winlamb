# WinLamb

A lightweight modern C++17 library for [Win32 API](https://en.wikipedia.org/wiki/Windows_API), using [lambda closures](https://www.cprogramming.com/c++11/c++11-lambda-closures.html) to handle window messages.

**Note:** If you're looking for the old C++11, version, [see this branch](https://github.com/rodrigocfd/winlamb/tree/cpp11).

## Usage

WinLamb is a header-only library. You can clone the repository or simply [download](https://github.com/rodrigocfd/winlamb/archive/master.zip) the files. Once referenced in your source code, it should work right away.

There's an included `win10.exe.manifest` file, which includes Common Controls and [Windows 10 support](https://docs.microsoft.com/pt-br/windows/desktop/SysInfo/targeting-your-application-at-windows-8-1).

## Example

This is a simple Win32 program written with WinLamb. Each window has a class, and messages are handled with lambda closures. There's no need to write a message loop or window registering.

* Declaration file: `My_Window.h`

````cpp
#include "winlamb/window_main.h"

class My_Window : public wl::window_main {
public:
    My_Window();
};
````

* Implementation file: `My_Window.cpp`

````cpp
#include "My_Window.h"

RUN(My_Window) // optional, generate WinMain call and instantiate MyWindow

MyWindow::My_Window()
{
    setup().title = L"Hello world";
    setup().style |= WS_MINIMIZEBOX;

    on_message(WM_CREATE, [this](wl::param::wm::create p) -> LRESULT
    {
        set_title(L"A new title for my window");
        return 0;
    });

    on_message(WM_LBUTTONDOWN, [](wl::param::wm::lbuttondown p) -> LRESULT
    {
        bool isCtrlDown = p.has_ctrl();
        long xPos = p.pos().x;
        return 0;
    });
}
````

Note that Win32 uses [Unicode strings](https://docs.microsoft.com/en-us/windows/win32/learnwin32/working-with-strings) natively, that means to use `wchar_t`, `std::wstring` and `std::wstring_view`.

## Classes summary

Most `.h` files are named after the class or namespace it contains, just include it.

* To create your [window](https://docs.microsoft.com/en-us/windows/win32/winmsg/using-window-procedures), inherit from the classes below:

| Class | Description | Examples |
| :--- | :--- | :--- |
| [`window_control`](window_control.h?ts=4) | An user-custom window control. | |
| [`window_main`](window_main.h?ts=4) | An ordinary main window for your application. | |
| [`window_modal`](window_modal.h?ts=4) | A modal popup window. | |
| [`window_modeless`](window_modeless.h?ts=4) | A modeless popup window. | |

* Or, using a [dialog resource](https://docs.microsoft.com/en-us/windows/win32/dlgbox/dlgbox-programming-considerations) instead of a raw window:

| Class | Description | Examples |
| :--- | :--- | :--- |
| [`dialog_control`](dialog_control.h?ts=4) | A dialog to be used as a control within a parent window. | |
| [`dialog_main`](dialog_main.h?ts=4) | A dialog as the main window for your application. | |
| [`dialog_modal`](dialog_modal.h?ts=4) | A modal dialog popup. | |
| [`dialog_modeless`](dialog_modeless.h?ts=4) | A modeless dialog popup. | |

* Native controls:

| Class | Description | Examples |
| :--- | :--- | :--- |
| [`button`](button.h?ts=4) | Native [button](https://docs.microsoft.com/en-us/windows/win32/controls/button-types-and-styles#push-buttons) control. | |
| [`check_box`](check_box.h?ts=4) | Native [check box](https://docs.microsoft.com/en-us/windows/win32/controls/button-types-and-styles#check-boxes) control. | |
| [`combo_box`](combo_box.h?ts=4) | Native [combo box](https://docs.microsoft.com/en-us/windows/win32/controls/about-combo-boxes) (dropdown) control. | |
| [`date_time_picker`](date_time_picker.h?ts=4) | Native [date and time picker](https://docs.microsoft.com/en-us/windows/win32/controls/date-and-time-picker-controls) (calendar) control. | |
| [`edit`](edit.h?ts=4) | Native [edit](https://docs.microsoft.com/en-us/windows/win32/controls/about-edit-controls) (textbox) control. | |
| [`label`](label.h?ts=4) | Native [static](https://docs.microsoft.com/en-us/windows/win32/controls/about-static-controls) (label) control. | |
| [`list_view`](list_view.h?ts=4) | Native [list view](https://docs.microsoft.com/en-us/windows/win32/controls/list-view-controls-overview) control. | |
| [`menu_popup`](menu.h?ts=4) | Manages an `HMENU` resource of a popup menu. Calls `DestroyMenu()` on destructor. | |
| [`progress_bar`](progress_bar.h?ts=4) | Native [progress bar](https://docs.microsoft.com/en-us/windows/win32/controls/progress-bar-control) control. Optionally reflects the progress in window taskbar. | |
| [`radio_group`](radio_group.h?ts=4) | Manages a group of native [radio button](https://docs.microsoft.com/en-us/windows/win32/controls/button-types-and-styles#radio-buttons) controls. | |
| [`status_bar`](status_bar.h?ts=4) | Native [status bar](https://docs.microsoft.com/en-us/windows/win32/controls/status-bars) control. | |
| [`sys_dlg`](sys_dlg.h?ts=4) | Native system dialogs. | |
| [`tree_view`](tree_view.h?ts=4) | Native [tree view](https://docs.microsoft.com/en-us/windows/win32/controls/tree-view-controls) control. | |

* Wrapper and utility classes:

| Class | Description | Examples |
| :--- | :--- | :--- |
| [`accel_table`](accel_table.h?ts=4) | Helps building an [accelerator table](https://docs.microsoft.com/en-us/windows/win32/learnwin32/accelerator-tables). | |
| [`bin`](bin.h?ts=4) | Utilities for binary/string conversions. | |
| [`com::bstr`](com.h?ts=4) | Manages a COM `BSTR` string. | |
| [`com::lib`](com.h?ts=4) | Automates `CoInitialize()` and `CoUninitialize()` calls with RAII. | |
| [`com::ptr`](com.h?ts=4) | Manages a COM pointer. | |
| [`com::variant`](com.h?ts=4) | Manages a COM `VARIANT` object. | |
| [`download`](download.h?ts=4) | Manages internet download operations with [WinHTTP](https://docs.microsoft.com/en-us/windows/win32/winhttp/about-winhttp). | |
| [`exe`](exe.h?ts=4) | Executable-related utilities. | |
| [`file`](file.h?ts=4) | Manages a low-level `HANDLE` to a file. | |
| [`file_ini`](file.h?ts=4) | Manages an INI file. | |
| [`file_mapped`](file.h?ts=4) | Manages a memory-mapped file. | |
| [`gdi`](gdi.h?ts=4) | Device context and GDI objects. | |
| [`icon`](icon.h?ts=4) | Manages an `HICON` resource. | |
| [`image_list`](image_list.h?ts=4) | Manages a native [image list](https://docs.microsoft.com/en-us/windows/win32/controls/image-lists). | |
| [`insert_order_map`](insert_order_map.h?ts=4) | Vector-based associative container which keeps the insertion order. Uses linear search, suitable for few elements. | |
| [`path`](path.h?ts=4) | Utilities to file path operations. | |
| [`resizer`](resizer.h?ts=4) | Automates the resizing of multiple controls when the parent window is resized. | |
| [`str`](str.h?ts=4) | Utilities for `std::wstring`. | |
| [`time`](time.h?ts=4) | Utilities for `std::chrono`. | |
| [`xml`](xml.h?ts=4) | Handles XML documents using MSXML 6.0 library. | |
| [`zip`](zip.h?ts=4) | Zip file utilities. | |

## License

Licensed under [MIT license](https://opensource.org/licenses/MIT), see [LICENSE.txt](LICENSE.txt) for details.
