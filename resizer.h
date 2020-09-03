/**
 * Part of WinLamb - Win32 API Lambda Library
 * https://github.com/rodrigocfd/winlamb
 * This library is released under the MIT License
 */

#pragma once
#include <vector>
#include <Windows.h>
#include "internals/interfaces.h"
#include "internals/param.h"

namespace wl {

// Automates the resizing of multiple controls when the parent window is resized.
class resizer final {
public:
	enum class go {
		// Control size is fixed; control moves around anchored.
		REPOS,
		// Control size stretches; control doesn't move.
		RESIZE,
		// Control doesn't move or resize.
		NOTHING
	};

private:
	struct ctrl final {
		HWND hChild;
		RECT rcOrig;   // original coordinates relative to parent
		go   modeHorz; // horizontal mode
		go   modeVert; // vertical mode
	};

	std::vector<ctrl> _ctrls;
	SIZE _szOrig{};

public:
	resizer() = default;
	resizer(resizer&& other) = default;
	resizer& operator=(resizer&& other) = default; // movable only

	// Adds a new child control to be resized when parent resizes.
	resizer& add(const i_window* parent, int childId, go modeHorz, go modeVert)
	{
		this->_add_ctrl(GetDlgItem(parent->hwnd(), childId), modeHorz, modeVert);
		return *this;
	}

	// Adds a new child control to be resized when parent resizes.
	resizer& add(const i_window* parent,
		std::initializer_list<int> childIds, go modeHorz, go modeVert)
	{
		this->_ctrls.reserve(this->_ctrls.size() + childIds.size());
		for (int childId : childIds) {
			this->add(parent, childId, modeHorz, modeVert);
		}
		return *this;
	}

	// Adds a new child control to be resized when parent resizes.
	resizer& add(const i_control& child, go modeHorz, go modeVert)
	{
		this->_add_ctrl(child.hwnd(), modeHorz, modeVert);
		return *this;
	}

	// Adds a new child control to be resized when parent resizes.
	resizer& add(
		std::initializer_list<std::reference_wrapper<const i_control>> children,
		go modeHorz, go modeVert)
	{
		this->_ctrls.reserve(this->_ctrls.size() + children.size());
		for (const i_control& child : children) {
			this->add(child, modeHorz, modeVert);
		}
		return *this;
	}

	// Updates controls, intended to be called with parent's WM_SIZE processing.
	void adjust(const param::wm::size p) noexcept
	{
		if (this->_ctrls.empty() || p.is_minimized()) {
			return; // if no controls, or if minimized, no need to process
		}

		HDWP hdwp = BeginDeferWindowPos(static_cast<int>(this->_ctrls.size()));
		for (const ctrl& control : this->_ctrls) {
			UINT uFlags = SWP_NOZORDER;
			if (control.modeHorz == go::REPOS && control.modeVert == go::REPOS) { // reposition both vert & horz
				uFlags |= SWP_NOSIZE;
			} else if (control.modeHorz == go::RESIZE && control.modeVert == go::RESIZE) { // resize both vert & horz
				uFlags |= SWP_NOMOVE;
			}

			DeferWindowPos(hdwp, control.hChild, nullptr,
				control.modeHorz == go::REPOS ?
				p.sz().cx - this->_szOrig.cx + control.rcOrig.left :
				control.rcOrig.left, // keep original pos
				control.modeVert == go::REPOS ?
				p.sz().cy - this->_szOrig.cy + control.rcOrig.top :
				control.rcOrig.top, // keep original pos
				control.modeHorz == go::RESIZE ?
				p.sz().cx - this->_szOrig.cx + control.rcOrig.right - control.rcOrig.left :
				control.rcOrig.right - control.rcOrig.left, // keep original width
				control.modeVert == go::RESIZE ?
				p.sz().cy - this->_szOrig.cy + control.rcOrig.bottom - control.rcOrig.top :
				control.rcOrig.bottom - control.rcOrig.top, // keep original height
				uFlags);
		}
		EndDeferWindowPos(hdwp);
	}

private:
	void _add_ctrl(HWND hCtrl, go modeHorz, go modeVert)
	{
		HWND hParent = GetParent(hCtrl);
		if (this->_ctrls.empty()) { // first call to _addOne()
			RECT rcP{};
			GetClientRect(hParent, &rcP);
			this->_szOrig.cx = rcP.right;
			this->_szOrig.cy = rcP.bottom; // save original size of parent
		}

		RECT rcCtrl{};
		GetWindowRect(hCtrl, &rcCtrl);
		this->_ctrls.push_back({hCtrl, rcCtrl, modeHorz, modeVert});
		ScreenToClient(hParent, reinterpret_cast<POINT*>(&this->_ctrls.back().rcOrig)); // client coordinates relative to parent
		ScreenToClient(hParent, reinterpret_cast<POINT*>(&this->_ctrls.back().rcOrig.right));
	}
};


}//namespace wl