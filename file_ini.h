/**
 * Part of WinLamb - Win32 API Lambda Library
 * https://github.com/rodrigocfd/winlamb
 * This library is released under the MIT License
 */

#pragma once
#include <stdexcept>
#include <string_view>
#include <vector>
#include "bin.h"
#include "file.h"
#include "file_mapped.h"
#include "insert_order_map.h"
#include "bin.h"
#include "str.h"

namespace wl {

// Manages an INI file.
class file_ini final {
public:
	insert_order_map<std::wstring,
		insert_order_map<std::wstring, std::wstring>> sections;

	// Tells if the section/key exists, case-sensitive.
	[[nodiscard]] bool has(std::wstring_view section, std::wstring_view key) const noexcept
	{
		auto mySection = this->sections.find(section.data());
		if (mySection.has_value()) {
			return mySection.value().get()
				.find(key.data()).has_value();
		}
		return false;
	}

	// Loads a file and parses its contents.
	file_ini& load_from_file(std::wstring_view filePath)
	{
		file_mapped fin;
		fin.open_read(filePath);
		std::wstring contents = fin.read_as_string();
		fin.close();

		insert_order_map<std::wstring, std::wstring>* curSection = nullptr;
		std::wstring tmpKey, tmpVal; // temporary buffers

		for (std::wstring& line : str::split_lines(contents)) {
			str::trim(line);

			if (line.empty()) { // skip blank lines
				continue;

			} else if (line[0] == L'[' && line.back() == L']') { // begin of section found
				tmpKey.clear();
				tmpKey.insert(0, &line[1], line.length() - 2); // extract section name
				curSection = &this->sections[str::trim(tmpKey)]; // if inexistent, will be inserted

			} else if (curSection && line[0] != L';' && line[0] != L'#') { // lines starting with ; or # will be ignored
				size_t idxEq = line.find_first_of(L'=');
				if (idxEq != std::wstring::npos) {
					tmpKey.clear();
					tmpKey.insert(0, &line[0], idxEq); // extract key name
					tmpVal.clear();
					tmpVal.insert(0, &line[idxEq + 1], line.length() - (idxEq + 1)); // extract value
					(*curSection)[tmpKey] = tmpVal;
				}
			}
		}

		return *this;
	}

	// Writes the INI contents as an UTF-8 file.
	const file_ini& save_to_file(std::wstring_view filePath) const
	{
		std::vector<BYTE> blob = bin::str_to_utf8(this->serialize());

		file fout;
		fout.open_or_create(filePath)
			.set_new_size(blob.size())
			.write(blob);

		return *this;
	}

	// Returns the INI contents as a string, ready to be written to a file.
	[[nodiscard]] std::wstring serialize() const
	{
		std::wstring out;
		bool isFirst = true;

		for (const auto& sectionEntry : this->sections) {
			if (isFirst) {
				isFirst = false;
			} else {
				out.append(L"\r\n");
			}
			out.append(L"[").append(sectionEntry.key).append(L"]\r\n");

			for (const auto& keyEntry : sectionEntry.val) {
				out.append(keyEntry.key).append(L"=")
					.append(keyEntry.val).append(L"\r\n");
			}
		}
		return out;
	}

	// Checks, case-sensitive, INI file structure against a model like:
	// "[section1]keyA,keyB,keyC[section2]keyX,keyY".
	[[nodiscard]] bool structure_is(std::wstring_view structure) const
	{
		auto ff = this->_parse_descr_structure(structure);

		for (const auto& parsedSection : this->_parse_descr_structure(structure)) {
			auto curSection = this->sections.find(parsedSection.key);
			if (!curSection.has_value()) { // no such section name
				return false;
			}

			for (const std::wstring& parsedEntry : parsedSection.val) {
				if (!curSection.value().get().contains(parsedEntry)) { // no such key name in section
					return false;
				}
			}
		}
		return true;
	}

	// Returns a reference to the value.
	// Throws an exception if section or key doesn't exist.
	[[nodiscard]] const std::wstring& val(std::wstring_view section, std::wstring_view key) const
	{
		auto mySection = this->sections.find(section.data());
		if (!mySection.has_value()) {
			throw std::invalid_argument(
				str::unicode_to_ansi(
					str::format(L"Section doesn't exist: \"%s\".", section) ));
		}

		auto myValue = mySection.value().get().find(key.data());
		if (!myValue.has_value()) {
			throw std::invalid_argument(
				str::unicode_to_ansi(
					str::format(L"Key doesn't exist in section \"%s\": \"%s\".", section, key) ));
		}

		return myValue.value().get();
	}

	// Returns a reference to the value.
	// Throws an exception if section or key doesn't exist.
	[[nodiscard]] std::wstring& val(std::wstring_view section, std::wstring_view key)
	{
		// https://stackoverflow.com/a/856839/6923555
		return const_cast<std::wstring&>(
			const_cast<const file_ini*>(this)->val(section, key) );
	}

private:
	[[nodiscard]] insert_order_map<std::wstring, std::vector<std::wstring>>
		_parse_descr_structure(std::wstring_view structure) const
	{
		insert_order_map<std::wstring,
			std::vector<std::wstring>> parsed;

		for (std::wstring& secBlock : str::split(structure, L"[")) {
			if (secBlock.empty()) {
				continue;
			}
			size_t endSecIdx = secBlock.find_first_of(L']');
			std::vector<std::wstring>& curSec = parsed[secBlock.substr(0, endSecIdx)];
			secBlock.erase(0, endSecIdx + 1);
			curSec = str::split(secBlock, L",");
		}

		return parsed;
	}
};

}//namespace wl