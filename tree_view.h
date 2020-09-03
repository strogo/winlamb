/**
 * Part of WinLamb - Win32 API Lambda Library
 * https://github.com/rodrigocfd/winlamb
 * This library is released under the MIT License
 */

#pragma once
#include <optional>
#include <Windows.h>
#include <CommCtrl.h>
#include "internals/base_native_control.h"
#include "internals/control_visuals.h"
#include "internals/interfaces.h"
#include "internals/tree_view_item.h"
#include "image_list.h"

namespace wl {

// Native tree view control.
// https://docs.microsoft.com/en-us/windows/win32/controls/tree-view-controls
class tree_view final : i_control {
private:
	_wli::base_native_control _base;

public:
	tree_view() = default;
	tree_view(tree_view&&) = default;
	tree_view& operator=(tree_view&&) = default; // movable only

	// Returns the HWND.
	[[nodiscard]] HWND hwnd() const noexcept override { return this->_base.hwnd(); }

	// Retrieves the control ID.
	[[nodiscard]] int id() const noexcept override { return this->_base.id(); }

	// Calls EnableWindow().
	const tree_view& enable(bool isEnabled) const noexcept { EnableWindow(this->hwnd(), isEnabled); return *this; }

	// In a dialog window, assigns to a control.
	tree_view& assign(const i_window* parent, int ctrlId) { this->_base.assign(parent, ctrlId); return *this; }

	// Installs a window subclass and adds a handle to a message.
	// func: [](wl::param::any p) -> LRESULT { }
	template<typename F>
	void on_subclass_message(UINT msg, F&& func) { this->_base.on_subclass_message(msg, std::move(func)); }

	// Installs a window subclass and adds a handle to a message.
	// func: [](wl::param::any p) -> LRESULT { }
	template<typename F>
	void on_subclass_message(std::initializer_list<UINT> msgs, F&& func) { this->_base.on_subclass_message(msgs, std::move(func)); }

	// Calls CreateWindowEx().
	// Should be called during parent's WM_CREATE processing.
	// Position and size will be adjusted to match current system DPI.
	tree_view& create(const i_window* parent, int id, POINT pos, SIZE size,
		DWORD tvStyles = TVS_HASLINES | TVS_LINESATROOT | TVS_SHOWSELALWAYS | TVS_HASBUTTONS,
		DWORD tvExStyles = 0)
	{
		HWND h = this->_base.create_window(parent, id, WC_TREEVIEWW, {}, pos, size,
			WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_GROUP | tvStyles,
			WS_EX_CLIENTEDGE);

		pos = _wli::multiply_dpi(pos);
		size = _wli::multiply_dpi(size);

		if (tvExStyles) {
			this->set_extended_tv_style(true, tvExStyles);
		}
		return *this;
	}

	// Adds a new root node, returning it.
	tree_view_item add_root_item(std::wstring_view text) const
	{
		TVINSERTSTRUCTW tvi{};
		tvi.hParent = TVI_ROOT;
		tvi.hInsertAfter = TVI_LAST;
		tvi.itemex.mask = TVIF_TEXT;
		tvi.itemex.pszText = const_cast<wchar_t*>(text.data());

		HTREEITEM newItem = TreeView_InsertItem(this->hwnd(), &tvi);
		if (!newItem) {
			throw std::runtime_error(
				wl::str::unicode_to_ansi(
					wl::str::format(L"TVM_INSERTITEM failed \"%s\".", text) ));
		}
		return tree_view_item{this->hwnd(), newItem};
	}

	// Adds a new root node, returning it.
	// You must attach an image list to see the icons.
	tree_view_item add_root_item_with_icon(std::wstring_view text, int iIcon) const
	{
		TVINSERTSTRUCTW tvi{};
		tvi.hParent = TVI_ROOT;
		tvi.hInsertAfter = TVI_LAST;
		tvi.itemex.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
		tvi.itemex.pszText = const_cast<wchar_t*>(text.data());
		tvi.itemex.iImage = iIcon;
		tvi.itemex.iSelectedImage = iIcon;

		HTREEITEM newItem = TreeView_InsertItem(this->hwnd(), &tvi);
		if (!newItem) {
			throw std::runtime_error(
				wl::str::unicode_to_ansi(
					wl::str::format(L"TVM_INSERTITEM failed \"%s\".", text) ));
		}
		return tree_view_item{this->hwnd(), newItem};
	}

	// Deletes all nodes.
	const tree_view& delete_all_items() const
	{
		if (!TreeView_DeleteAllItems(this->hwnd())) {
			throw std::runtime_error("TVM_DELETEITEM failed to delete all items.");
		}
		return *this;
	}

	// Retrieves the TVS_EX style.
	[[nodiscard]] size_t extended_tv_style() const noexcept
	{
		return SendMessageW(this->hwnd(), TVM_GETEXTENDEDSTYLE, 0, 0);
	}

	// Retrieves the first root node, if any.
	[[nodiscard]] std::optional<tree_view_item> first_root() const noexcept
	{
		HTREEITEM hti = TreeView_GetRoot(this->hwnd());
		if (hti) {
			return tree_view_item{this->hwnd(), hti};
		}
		return {};
	}

	// Retrieves the first visible node, if any.
	[[nodiscard]] std::optional<tree_view_item> first_visible() const noexcept
	{
		HTREEITEM hti = TreeView_GetFirstVisible(this->hwnd());
		if (hti) {
			return tree_view_item{this->hwnd(), hti};
		}
		return {};
	}

	// Returns the total number of nodes.
	[[nodiscard]] size_t item_count() const noexcept
	{
		return TreeView_GetCount(this->hwnd());
	}

	// Retrieves the last visible node, if any.
	[[nodiscard]] std::optional<tree_view_item> last_visible() const noexcept
	{
		HTREEITEM hti = TreeView_GetLastVisible(this->hwnd());
		if (hti) {
			return tree_view_item{this->hwnd(), hti};
		}
		return {};
	}

	// Retrieves all root nodes.
	[[nodiscard]] std::vector<tree_view_item> root_items() const
	{
		std::vector<tree_view_item> roots;

		HTREEITEM hti = TreeView_GetRoot(this->hwnd());
		while (hti) {
			roots.emplace_back(this->hwnd(), hti);
			hti = TreeView_GetNextSibling(this->hwnd(), hti);
		}
		return roots;
	}

	// Retrieves the selected node, if any.
	[[nodiscard]] std::optional<tree_view_item> selected_item() const noexcept
	{
		HTREEITEM hti = TreeView_GetSelection(this->hwnd());
		if (hti) {
			return tree_view_item{this->hwnd(), hti};
		}
		return {};
	}

	// Sets the TVS_EX style signaled by the TVS_EX mask.
	const tree_view& set_extended_tv_style(
		bool isSet, DWORD tvExStyles) const noexcept
	{
		SendMessageW(this->hwnd(), TVM_SETEXTENDEDSTYLE,
			tvExStyles, isSet ? tvExStyles : 0);
		return *this;
	}

	// Sets the associated image list with TreeView_SetImageList().
	// The imageList object is shared, and must remain valid.
	tree_view& set_image_list(
		const wl::image_list& imgList, DWORD tvsilType = TVSIL_NORMAL) noexcept
	{
		// This method is non-const because it's usually called during object creation,
		// which chains many non-const methods.
		TreeView_SetImageList(this->hwnd(), imgList.himagelist(), tvsilType);
		return *this;
	}

	// Retrieves the number of visible nodes.
	[[nodiscard]] size_t visible_item_count() const noexcept
	{
		return TreeView_GetVisibleCount(this->hwnd());
	}
};

}//namespace wl