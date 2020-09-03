/**
 * Part of WinLamb - Win32 API Lambda Library
 * https://github.com/rodrigocfd/winlamb
 * This library is released under the MIT License
 */

#pragma once
#include <optional>
#include <string_view>
#include <vector>
#include <Windows.h>
#include "../insert_order_map.h"

namespace wl {

// A single XML node.
class xml_node final {
public:
	// The name of this XML element.
	std::wstring name;
	// The textual content of the element.
	std::wstring text;
	// The attributes of this element.
	insert_order_map<std::wstring, std::wstring> attrs;
	// The child nodes of this element.
	std::vector<xml_node> children;

	// Returns a vector of references to the nodes with the given name, case insensitive.
	std::vector<std::reference_wrapper<const xml_node>>
		children_by_name(std::wstring_view elemName) const
	{
		return _children_by_name<const xml_node>(this, elemName);
	}

	// Returns a vector of references to the nodes with the given name, case insensitive.
	std::vector<std::reference_wrapper<xml_node>>
		children_by_name(std::wstring_view elemName)
	{
		return _children_by_name<xml_node>(this, elemName);
	}

	// Returns the first child with the given name, case insensitive.
	std::optional<std::reference_wrapper<const xml_node>>
		first_child_by_name(std::wstring_view elemName) const
	{
		return _first_child_by_name<const xml_node>(this, elemName);
	}

	// Returns the first child with the given name, case insensitive.
	std::optional<std::reference_wrapper<xml_node>>
		first_child_by_name(std::wstring_view elemName)
	{
		return _first_child_by_name<xml_node>(this, elemName);
	}

private:
	template<typename thisT>
	[[nodiscard]] static std::vector<std::reference_wrapper<thisT>>
		_children_by_name(thisT* thiss, std::wstring_view elemName)
	{
		// https://stackoverflow.com/a/11655924/6923555

		std::vector<std::reference_wrapper<thisT>> nodes;
		for (thisT& n : thiss->children) {
			if (!lstrcmpiW(n.name.c_str(), elemName.data())) {
				nodes.emplace_back(n);
			}
		}
		return nodes;
	}

	template<typename thisT>
	[[nodiscard]] static std::optional<std::reference_wrapper<thisT>>
		_first_child_by_name(thisT* thiss, std::wstring_view elemName)
	{
		for (thisT& n : thiss->children) {
			if (!lstrcmpiW(n.name.c_str(), elemName.data())) {
				return {n};
			}
		}
		return {};
	}
};

}//namespace wl