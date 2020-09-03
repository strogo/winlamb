/**
 * Part of WinLamb - Win32 API Lambda Library
 * https://github.com/rodrigocfd/winlamb
 * This library is released under the MIT License
 */

#pragma once
#include <optional>
#include <string_view>
#include <system_error>
#include <Windows.h>
#include <MsXml6.h>
#include "internals/xml_node.h"
#include "com.h"
#include "insert_order_map.h"
#pragma comment(lib, "msxml6.lib")

namespace wl {

// Handles XML documents using MSXML 6.0 library.
class xml final {
private:
	com::lib _comLib{com::lib::init::LATER};

public:
	// Root node of this XML document.
	xml_node root;

	xml() = default;
	xml(std::wstring_view xmlString) { this->parse(xmlString); }
	xml(xml&& other) noexcept { operator=(std::move(other)); } // movable only
	xml& operator=(xml&& other) = default;

	// Parses an XML string and loads it in memory.
	xml& parse(std::wstring_view xmlString)
	{
		this->_comLib.initialize();
		this->root = {};

		// Create COM object for XML document.
		auto doc = com::co_create_instance<IXMLDOMDocument3>(
			CLSID_DOMDocument60, CLSCTX_INPROC_SERVER);
		doc->put_async(false);

		// Parse the XML string.
		VARIANT_BOOL vb = FALSE;
		HRESULT hr = doc->loadXML(
			static_cast<BSTR>(const_cast<wchar_t*>(xmlString.data())), &vb);
		if (FAILED(hr)) {
			throw std::system_error(hr, std::system_category(),
				"IXMLDOMDocument3::loadXML failed.");
		}

		// Get document element and root node.
		com::ptr<IXMLDOMElement> docElem;
		hr = doc->get_documentElement(docElem.raw_pptr());
		if (FAILED(hr)) {
			throw std::system_error(hr, std::system_category(),
				"IXMLDOMDocument3::get_documentElement failed.");
		}

		auto rootNode = docElem.query_interface<IXMLDOMNode>();
		this->root = _build_node(rootNode);
		return *this;
	}

private:
	static xml_node _build_node(com::ptr<IXMLDOMNode>& xmlDomNode)
	{
		xml_node myNode;

		// Get node name.
		com::bstr bstrName;
		HRESULT hr = xmlDomNode->get_nodeName(&bstrName);
		if (FAILED(hr)) {
			throw std::system_error(hr, std::system_category(),
				"IXMLDOMNode::get_nodeName failed.");
		}
		myNode.name = bstrName.c_str();

		// Parse attributes, if any.
		myNode.attrs = _parse_attributes(xmlDomNode);

		// Process children, if any.
		_parse_children(xmlDomNode, myNode);

		return myNode;
	}

	[[nodiscard]] static insert_order_map<std::wstring, std::wstring>
		_parse_attributes(com::ptr<IXMLDOMNode>& xmlDomNode)
	{
		com::ptr<IXMLDOMNamedNodeMap> attrs;
		xmlDomNode->get_attributes(attrs.raw_pptr());

		long attrCount = 0;
		attrs->get_length(&attrCount);

		insert_order_map<std::wstring, std::wstring> myAttrs;
		myAttrs.reserve(attrCount);

		for (long i = 0; i < attrCount; ++i) {
			com::ptr<IXMLDOMNode> attr;
			attrs->get_item(i, attr.raw_pptr());

			DOMNodeType type = NODE_INVALID;
			attr->get_nodeType(&type);
			if (type == NODE_ATTRIBUTE) {
				com::bstr bstrName;
				attr->get_nodeName(&bstrName); // get attribute name
				com::variant variNodeVal;
				attr->get_nodeValue(&variNodeVal); // get attribute value
				myAttrs[bstrName.c_str()] = variNodeVal.str(); // add hash entry
			}
		}
		return myAttrs;
	}

	static void _parse_children(com::ptr<IXMLDOMNode>& xmlDomNode, xml_node& myNode)
	{
		VARIANT_BOOL vb = FALSE;
		xmlDomNode->hasChildNodes(&vb);

		if (vb) {
			com::ptr<IXMLDOMNodeList> nodeList;
			HRESULT hr = xmlDomNode->get_childNodes(nodeList.raw_pptr());
			if (FAILED(hr)) {
				throw std::system_error(hr, std::system_category(),
					"IXMLDOMNode::get_childNodes failed.");
			}

			int childCount = 0;
			long totalCount = 0;
			nodeList->get_length(&totalCount);

			for (long i = 0; i < totalCount; ++i) {
				com::ptr<IXMLDOMNode> child;
				hr = nodeList->get_item(i, child.raw_pptr());
				if (FAILED(hr)) {
					throw std::system_error(hr, std::system_category(),
						"IXMLDOMNodeList::get_item failed.");
				}

				// Node can be text or an actual child node.
				DOMNodeType type = NODE_INVALID;
				hr = child->get_nodeType(&type);
				if (FAILED(hr)) {
					throw std::system_error(hr, std::system_category(),
						"IXMLDOMNode::get_nodeType failed.");
				}

				if (type == NODE_TEXT) {
					com::bstr bstrText;
					xmlDomNode->get_text(&bstrText);
					myNode.text.append(bstrText.c_str()); // if text, append to current node text
				} else if (type == NODE_ELEMENT) {
					myNode.children.emplace_back(_build_node(child)); // recursively
				} else {
					// (L"Unhandled node type: %d.\n", type);
				}
			}
		} else {
			// Assumes that only a leaf node can have text.
			com::bstr bstrText;
			xmlDomNode->get_text(&bstrText); // if text, append to current node text
			myNode.text = bstrText.c_str();
		}
	}
};

}//namespace wl