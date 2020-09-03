/**
 * Part of WinLamb - Win32 API Lambda Library
 * https://github.com/rodrigocfd/winlamb
 * This library is released under the MIT License
 */

#pragma once
#include <optional>
#include <stdexcept>
#include <vector>
#include <Windows.h>
#include "internals/radio_button.h"

namespace wl {

// Manages a group of native radio button controls.
// https://docs.microsoft.com/en-us/windows/win32/controls/button-types-and-styles#radio-buttons
class radio_group final {
private:
	std::vector<radio_button> _items;

public:
	radio_group() = default;
	radio_group(radio_group&&) = default;
	radio_group& operator=(radio_group&&) = default; // movable only

	// Tells if the radio group is empty.
	[[nodiscard]] bool empty() const noexcept { return this->_items.empty(); }

	// Returns the number of radio controls in this radio group.
	[[nodiscard]] size_t size() const noexcept { return this->_items.size(); }

	[[nodiscard]] const radio_button& operator[](size_t index) const noexcept { return this->_items[index]; }
	[[nodiscard]] radio_button&       operator[](size_t index) noexcept       { return this->_items[index]; }

	// In a dialog window, assign to controls.
	radio_group& assign(i_window* parent, std::initializer_list<int> radioIds)
	{
		if (!this->empty()) {
			throw std::logic_error("Cannot assign a radio group twice.");
		}

		this->_items.reserve(radioIds.size());
		for (int radioId : radioIds) {
			radio_button newRadio{};
			newRadio.assign(parent, radioId);
			this->_items.emplace_back(std::move(newRadio));
		}
		return *this;
	}

	// Calls CreateWindowEx() to add a new radio button control.
	// Should be called during parent's WM_CREATE processing.
	// Position will be adjusted to match current system DPI.
	radio_group& add_create(const i_window* parent, int id,
		std::wstring_view text, POINT pos)
	{
		radio_button::type t = this->empty()
			? radio_button::type::FIRST
			: radio_button::type::NONFIRST;

		radio_button newRadio{};
		newRadio.create(parent, id, t, text, pos);
		this->_items.emplace_back(std::move(newRadio));
		return *this;
	}

	// Returns the radio button with the given control ID, if any.
	[[nodiscard]] std::optional<std::reference_wrapper<const radio_button>>
		by_id(int radioId) const noexcept
	{
		return _by_id<const radio_group, const radio_button>(this, radioId);
	}

	// Returns the radio button with the given control ID, if any.
	[[nodiscard]] std::optional<std::reference_wrapper<radio_button>>
		by_id(int radioId) noexcept
	{
		return _by_id<radio_group, radio_button>(this, radioId);
	}

	// Returns the currently checked radio button, if any.
	[[nodiscard]] std::optional<std::reference_wrapper<const radio_button>>
		checked() const noexcept
	{
		return _checked<const radio_group, const radio_button>(this);
	}

	// Returns a reference to the currently checked radio button.
	[[nodiscard]] std::optional<std::reference_wrapper<radio_button>>
		checked() noexcept
	{
		return _checked<radio_group, radio_button>(this);
	}

	// Returns the ID of the currently checked radio button.
	[[nodiscard]] std::optional<int> checked_id() const noexcept
	{
		auto checkedRadio = this->checked();
		if (checkedRadio.has_value()) {
			return checkedRadio.value().get().id();
		}
		return {};
	}

	// Enables or disables all radio buttons at once.
	const radio_group& enable(bool isEnabled) const noexcept
	{
		for (const radio_button& rb : this->_items) {
			rb.enable(isEnabled);
		}
		return *this;
	}

private:
	template<typename thisT, typename retT>
	[[nodiscard]] static std::optional<std::reference_wrapper<retT>>
		_by_id(thisT* thiss, int radioId) noexcept
	{
		// https://stackoverflow.com/a/11655924/6923555

		for (retT& rad : thiss->_items) {
			if (rad.id() == radioId) {
				return {rad};
			}
		}
		return {};
	}

	template<typename thisT, typename retT>
	[[nodiscard]] static std::optional<std::reference_wrapper<retT>>
		_checked(thisT* thiss) noexcept
	{
		for (retT& rad : thiss->_items) {
			if (rad.checked()) {
				return {rad};
			}
		}
		return {};
	}
};

}//namespace wl