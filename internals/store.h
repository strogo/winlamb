/**
 * Part of WinLamb - Win32 API Lambda Library
 * https://github.com/rodrigocfd/winlamb
 * This library is released under the MIT License
 */

#pragma once
#include <functional>
#include <vector>
#include <Windows.h>
#include "param.h"

namespace _wli {

// Generic storage for message identifiers and their respective lambda handlers.
template<typename idT>
class store final {
private:
	struct handler final {
		idT id; // UINT, WORD or std::pair<UINT_PTR, int>
		std::function<LRESULT(wl::param::any)> func; // user handler; returned INT_PTR is convertible to LRESULT

		template<typename F>
		handler(idT id, F&& func) noexcept : // needed for emplace_back(id, func)
			id{id}, func{std::move(func)} { }
	};

	std::vector<handler> _handlers;

public:
	store()
	{
		// This works for UINT and WORD.
		// For std::pair<UINT_PTR, UINT>, there's an specialization after this class.

		this->_handlers.emplace_back(0, nullptr); // add 1st element, which is room for sentinel
	}

	[[nodiscard]] bool empty() const noexcept
	{
		return this->_handlers.size() <= 1; // sentinel element always present
	}

	template<typename F>
	void add(idT id, F&& func)
	{
		this->_handlers.emplace_back(id, std::move(func));
	}

	template<typename F>
	void add(std::initializer_list<idT> ids, F&& func)
	{
		const idT* pIds = ids.begin();
		this->add(pIds[0], std::move(func)); // store user func once, under 1st ID
		size_t funcIdx = this->_handlers.size() - 1; // internal index of stored user func
		for (size_t i = 1; i < ids.size(); ++i) { // for all remaining IDs
			if (pIds[i] != pIds[0]) { // avoid overwriting
				this->add(pIds[i], [this, funcIdx](wl::param::any p) -> LRESULT { // store light wrapper to 1st func
					return this->_handlers[funcIdx].func(p);
				});
			}
		}
	}

	[[nodiscard]] std::function<LRESULT(wl::param::any)>* find(idT id) noexcept
	{
		this->_handlers[0].id = id; // sentinel for reverse linear search
		handler* revRunner = &this->_handlers.back(); // pointer to last element
		while (revRunner->id != id) --revRunner;
		return revRunner == &this->_handlers[0]
			? nullptr // if we stopped only at 1st element (sentinel), id wasn't found
			: &revRunner->func; // handler found
	}
};

template<>
inline store<std::pair<UINT_PTR, int>>::store()
{
	// Constructor specialization to deal with std::pair<UINT_PTR, int> empty sentinel init.

	this->_handlers.emplace_back(std::make_pair(0, 0), nullptr); // add 1st element, which is room for sentinel
}

}//namespace _wli