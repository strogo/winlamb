/**
 * Part of WinLamb - Win32 API Lambda Library
 * https://github.com/rodrigocfd/winlamb
 * This library is released under the MIT License
 */

#pragma once
#include <stdexcept>
#include <string_view>
#include <Windows.h>
#include <ShlObj.h>
#include "com.h"
#include "path.h"

// Zip file utilities.
namespace wl::zip {

inline void extract_all(std::wstring_view zipFile, std::wstring_view destFolder)
{
	if (!path::exists(zipFile)) {
		throw std::invalid_argument("File doesn't exist.");
	}
	if (!path::exists(destFolder)) {
		throw std::invalid_argument("Output directory doesn't exist.");
	}

	// http://social.msdn.microsoft.com/Forums/vstudio/en-US/45668d18-2840-4887-87e1-4085201f4103/visual-c-to-unzip-a-zip-file-to-a-specific-directory
	com::lib comLib{com::lib::init::NOW};

	auto shellDispatch = com::co_create_instance<IShellDispatch>(
		CLSID_Shell, IID_IShellDispatch, CLSCTX_INPROC_SERVER);

	com::variant variZipFilePath;
	variZipFilePath.set_str(zipFile);

	com::ptr<Folder> zippedFile;
	if (FAILED(shellDispatch->NameSpace(variZipFilePath, zippedFile.raw_pptr()))) {
		throw std::runtime_error("IShellDispatch::NameSpace failed on zip file name.");
	}

	com::variant variOutFolderPath;
	variOutFolderPath.set_str(destFolder);

	com::ptr<Folder> outFolder;
	if (FAILED(shellDispatch->NameSpace(variOutFolderPath, outFolder.raw_pptr()))) {
		throw std::runtime_error("IShellDispatch::NameSpace failed on directory name.");
	}

	com::ptr<FolderItems> filesInside;
	if (FAILED(zippedFile->Items(filesInside.raw_pptr()))) {
		throw std::runtime_error("Folder::Items failed.");
	}

	long fileCount = 0;
	if (FAILED(filesInside->get_Count(&fileCount))) {
		throw std::runtime_error("Folder::get_Count failed.");
	}

	com::variant variItem;
	variItem.set_idispatch(filesInside);

	com::variant variOptions;
	variOptions.set_int32(1024 | 512 | 16 | 4); // https://docs.microsoft.com/en-us/windows/win32/shell/folder-copyhere

	if (FAILED(outFolder->CopyHere(variItem, variOptions))) {
		throw std::runtime_error("Folder::CopyHere failed.");
	}
}

}//namespace wl::zip