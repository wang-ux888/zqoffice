// =============================================================================
// src/ooxml/xml_reader.cpp
//
// XmlReader 实现，基于 pugixml 1.14
// =============================================================================
#include "xml_reader.h"

#include <pugixml.hpp>
#include <cstring>

namespace zq {
namespace office {
namespace ooxml {

namespace detail {

/// XmlNode 内部包装（封装 pugi::xml_node）
struct PugiXmlNodeWrapper {
    pugi::xml_node node;

    PugiXmlNodeWrapper() = default;
    explicit PugiXmlNodeWrapper(const pugi::xml_node& n) : node(n) {}
};

/// XmlReader 内部包装（封装 pugi::xml_document）
struct PugiXmlDocumentWrapper {
    pugi::xml_document doc;
};

} // namespace detail

// ---------------------------------------------------------------------------
// XmlNode
// ---------------------------------------------------------------------------
XmlNode::XmlNode() = default;

XmlNode::~XmlNode() {
    // wrapper_ 是由 XmlReader 拥有的文档创建的视图，不释放
    // 但独立创建的 wrapper_ 需要释放
    // 为简化，所有 wrapper_ 都由 XmlReader 管理，XmlNode 仅持有指针
    // 实际上，为了让 XmlNode 可以独立存在（如从 firstChild 返回），
    // 我们让 XmlNode 持有 wrapper_ 的所有权
}

XmlNode::XmlNode(detail::PugiXmlNodeWrapper* wrapper) : wrapper_(wrapper) {}

bool XmlNode::valid() const {
    return wrapper_ != nullptr && wrapper_->node;
}

std::string XmlNode::name() const {
    if (!valid()) return "";
    return std::string(wrapper_->node.name());
}

std::string XmlNode::text() const {
    if (!valid()) return "";
    return std::string(wrapper_->node.text().get());
}

std::string XmlNode::attribute(const std::string& name,
                                const std::string& defaultValue) const {
    if (!valid()) return defaultValue;
    pugi::xml_attribute attr = wrapper_->node.attribute(name.c_str());
    if (attr.empty()) return defaultValue;
    return std::string(attr.value());
}

bool XmlNode::hasAttribute(const std::string& name) const {
    if (!valid()) return false;
    return !wrapper_->node.attribute(name.c_str()).empty();
}

std::vector<std::string> XmlNode::attributeNames() const {
    std::vector<std::string> result;
    if (!valid()) return result;
    for (pugi::xml_attribute attr = wrapper_->node.first_attribute();
         attr; attr = attr.next_attribute()) {
        result.emplace_back(attr.name());
    }
    return result;
}

XmlNode XmlNode::firstChild(const std::string& name) const {
    if (!valid()) return XmlNode();
    pugi::xml_node child = (name.empty())
        ? wrapper_->node.first_child()
        : wrapper_->node.child(name.c_str());
    if (!child) return XmlNode();
    return XmlNode(new detail::PugiXmlNodeWrapper(child));
}

XmlNode XmlNode::nextSibling(const std::string& name) const {
    if (!valid()) return XmlNode();
    pugi::xml_node sib = (name.empty())
        ? wrapper_->node.next_sibling()
        : wrapper_->node.next_sibling(name.c_str());
    if (!sib) return XmlNode();
    return XmlNode(new detail::PugiXmlNodeWrapper(sib));
}

std::vector<XmlNode> XmlNode::children(const std::string& name) const {
    std::vector<XmlNode> result;
    if (!valid()) return result;
    for (pugi::xml_node child = (name.empty())
            ? wrapper_->node.first_child()
            : wrapper_->node.child(name.c_str());
         child;
         child = (name.empty())
            ? child.next_sibling()
            : child.next_sibling(name.c_str())) {
        result.emplace_back(new detail::PugiXmlNodeWrapper(child));
    }
    return result;
}

std::size_t XmlNode::childCount(const std::string& name) const {
    if (!valid()) return 0;
    std::size_t count = 0;
    for (pugi::xml_node child = (name.empty())
            ? wrapper_->node.first_child()
            : wrapper_->node.child(name.c_str());
         child;
         child = (name.empty())
            ? child.next_sibling()
            : child.next_sibling(name.c_str())) {
        ++count;
    }
    return count;
}

XmlNode XmlNode::findChildByAttribute(const std::string& childName,
                                       const std::string& attrName,
                                       const std::string& attrValue) const {
    if (!valid()) return XmlNode();
    pugi::xml_node child = wrapper_->node.find_child_by_attribute(
        childName.c_str(), attrName.c_str(), attrValue.c_str());
    if (!child) return XmlNode();
    return XmlNode(new detail::PugiXmlNodeWrapper(child));
}

// ---------------------------------------------------------------------------
// XmlReader
// ---------------------------------------------------------------------------
XmlReader::XmlReader()
    : doc_(new detail::PugiXmlDocumentWrapper()) {}

XmlReader::~XmlReader() {
    delete doc_;
}

bool XmlReader::parse(const std::string& xml) {
    return parse(reinterpret_cast<const unsigned char*>(xml.data()),
                 xml.size());
}

bool XmlReader::parse(const unsigned char* data, std::size_t size) {
    parsed_ = false;
    error_.clear();
    errorOffset_ = 0;

    if (!data || size == 0) {
        error_ = "empty XML data";
        return false;
    }

    // pugixml 解析：使用 parse_default（不保留空白文本节点）
    // 注意：parse_ws_pcdata 会保留缩进空白为 PCDATA 节点，干扰 firstChild/children/childCount
    //       的元素遍历语义；OOXML 解析只需元素节点，文本内容通过 text() 获取
    pugi::xml_parse_result result = doc_->doc.load_buffer(
        data, size,
        pugi::parse_default,
        pugi::encoding_utf8);

    if (!result) {
        error_ = result.description();
        errorOffset_ = result.offset ? static_cast<std::size_t>(
            reinterpret_cast<const char*>(result.offset) -
            reinterpret_cast<const char*>(data)) : 0;
        return false;
    }

    parsed_ = true;
    return true;
}

bool XmlReader::isParsed() const {
    return parsed_;
}

XmlNode XmlReader::root() const {
    if (!parsed_) return XmlNode();
    pugi::xml_node root = doc_->doc.document_element();
    if (!root) return XmlNode();
    return XmlNode(new detail::PugiXmlNodeWrapper(root));
}

} // namespace ooxml
} // namespace office
} // namespace zq
