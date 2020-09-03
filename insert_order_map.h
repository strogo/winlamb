/**
 * Part of WinLamb - Win32 API Lambda Library
 * https://github.com/rodrigocfd/winlamb
 * This library is released under the MIT License
 */

#pragma once
#include <optional>
#include <vector>

namespace wl {

// Vector-based associative container which keeps the insertion order.
// Uses linear search, suitable for few elements.
template<typename K, typename V>
class insert_order_map final {
public:
	struct entry final {
		K key;
		V val;

		entry() = default;
		explicit entry(const K& key) : key{key} { }
		entry(const K& key, const V& val) : key{key}, val{val} { }
	};

private:
	std::vector<entry> _entries;

public:
	insert_order_map() = default;
	insert_order_map(insert_order_map&&) = default;
	insert_order_map& operator=(insert_order_map&&) = default; // movable only

	using iterator               = typename std::vector<entry>::iterator;
	using const_iterator         = typename std::vector<entry>::const_iterator;
	using reverse_iterator       = typename std::vector<entry>::reverse_iterator;
	using const_reverse_iterator = typename std::vector<entry>::const_reverse_iterator;

	[[nodiscard]] iterator begin() noexcept { return this->_entries.begin(); }
	[[nodiscard]] iterator end() noexcept   { return this->_entries.end(); }
	[[nodiscard]] const_iterator begin() const noexcept  { return this->_entries.begin(); }
	[[nodiscard]] const_iterator end() const noexcept    { return this->_entries.end(); }
	[[nodiscard]] const_iterator cbegin() const noexcept { return this->_entries.cbegin(); }
	[[nodiscard]] const_iterator cend() const noexcept   { return this->_entries.cend(); }
	[[nodiscard]] reverse_iterator rbegin() noexcept { return this->_entries.rbegin(); }
	[[nodiscard]] reverse_iterator rend() noexcept   { return this->_entries.rend(); }
	[[nodiscard]] const_reverse_iterator rbegin() const noexcept  { return this->_entries.rbegin(); }
	[[nodiscard]] const_reverse_iterator rend() const noexcept    { return this->_entries.rend(); }
	[[nodiscard]] const_reverse_iterator crbegin() const noexcept { return this->_entries.crbegin(); }
	[[nodiscard]] const_reverse_iterator crend() const noexcept   { return this->_entries.crend(); }

	void                 clear() noexcept                       { this->_entries.clear(); }
	[[nodiscard]] bool   contains(const K& key) const noexcept  { return this->_find_idx(key) != -1; }
	[[nodiscard]] bool   empty() const noexcept                 { return this->_entries.empty(); }
	void                 reserve(size_t newCapacity)            { this->_entries.reserve(newCapacity); }
	[[nodiscard]] size_t size() const noexcept                  { return this->_entries.size(); }
	void                 swap(insert_order_map& other) noexcept { this->_entries.swap(other._entries); }

	// Returns a reference to the value with the given key.
	// If key doesn't exist, inserts it by calling emplace().
	V& operator[](const K& key)
	{
		size_t idx = this->_find_idx(key);
		if (idx == -1) {
			return this->emplace(key).first->val;
		}
		return this->_entries[idx].val;
	}

	// Inserts a new element into the container constructed in-place.
	// Returns a pair consisting of an iterator to the inserted element, or the
	// already-existing element if no insertion happened, and a bool denoting whether
	// the insertion took place (true if insertion happened, false if it did not).
	std::pair<iterator, bool> emplace(const K& key, const V& val)
	{
		size_t idx = this->_find_idx(key);
		if (idx == -1) {
			this->_entries.emplace_back(key, val);
			return {--this->end(), true};
		}
		return {this->begin() + idx, false};
	}

	// Inserts a new element into the container constructed in-place.
	// Returns a pair consisting of an iterator to the inserted element, or the
	// already-existing element if no insertion happened, and a bool denoting whether
	// the insertion took place (true if insertion happened, false if it did not).
	std::pair<iterator, bool> emplace(const K& key)
	{
		size_t idx = this->_find_idx(key);
		if (idx == -1) {
			this->_entries.emplace_back(key);
			return {--this->end(), true};
		}
		return {this->begin() + idx, false};
	}

	// Returns the element with the given key, if any.
	[[nodiscard]] std::optional<std::reference_wrapper<const V>>
		find(const K& key) const noexcept
	{
		return _find_val<const insert_order_map<K, V>, const V>(this, key);
	}

	// Returns the element with the given key, if any.
	[[nodiscard]] std::optional<std::reference_wrapper<V>>
		find(const K& key) noexcept
	{
		return _find_val<insert_order_map<K, V>, V>(this, key);
	}

private:
	[[nodiscard]] size_t _find_idx(const K& key) const noexcept
	{
		for (size_t i = 0; i < this->_entries.size(); ++i) {
			if (this->_entries[i].key == key) { // if string, will be case-sensitive
				return i;
			}
		}
		return -1;
	}

	template<typename thisT, typename retT>
	[[nodiscard]] static std::optional<std::reference_wrapper<retT>>
		_find_val(thisT* thiss, const K& key) noexcept
	{
		// https://stackoverflow.com/a/11655924/6923555

		size_t idx = thiss->_find_idx(key);
		if (idx == -1) {
			return {};
		}
		return {thiss->_entries[idx].val};
	}
};

}//namespace wl