// =============================================================================
// src/ooxml/xml_reader.h
//
// XML 读取器（DOM 模式）
//
//   基于 pugixml 1.14（轻量级 C++ XML 库，MIT 协议）：
//     - DOM 模式：一次性加载 XML 到内存，支持 XPath 查询
//     - 高性能：比 tinyxml2 快 5-10 倍
//     - 低内存：紧凑的内存表示
//
//   XmlNode 提供：
//     - 节点名称/文本/属性访问
//     - 子节点遍历（按名称过滤）
//     - 兄弟节点遍历
//     - XPath 查询
//
//   XmlReader 提供：
//     - 从字符串/字节流解析 XML
//     - 获取根节点
//     - 错误信息
// =============================================================================
#pragma once

#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>

namespace zq {
namespace office {
namespace ooxml {

// 前向声明（内部类型，避免暴露 pugixml 头文件）
namespace detail {
struct PugiXmlNodeWrapper;
struct PugiXmlDocumentWrapper;
} // namespace detail

/// XML 节点（DOM 节点视图）
///
///   - 视图语义：不持有数据，仅引用 XmlReader 拥有的文档
///   - 节点失效后访问属未定义行为（保证 XmlReader 生命周期）
class XmlNode {
public:
    XmlNode();
    ~XmlNode();

    /// 是否有效
    bool valid() const;

    /// 节点名称（如 "worksheet" / "sheetData" / "row"）
    std::string name() const;

    /// 节点文本内容（对于叶子节点）
    std::string text() const;

    /// 获取属性值
    /// @param name  属性名
    /// @param defaultValue  属性不存在时的默认值
    /// @return 属性值
    std::string attribute(const std::string& name,
                          const std::string& defaultValue = "") const;

    /// 是否有指定属性
    bool hasAttribute(const std::string& name) const;

    /// 获取所有属性名
    std::vector<std::string> attributeNames() const;

    /// 获取第一个子节点
    /// @param name  子节点名称过滤，空字符串表示任意名称
    XmlNode firstChild(const std::string& name = "") const;

    /// 获取下一个兄弟节点
    /// @param name  兄弟节点名称过滤，空字符串表示任意名称
    XmlNode nextSibling(const std::string& name = "") const;

    /// 获取所有子节点
    /// @param name  子节点名称过滤，空字符串表示任意名称
    std::vector<XmlNode> children(const std::string& name = "") const;

    /// 子节点数量
    /// @param name  子节点名称过滤，空字符串表示任意名称
    std::size_t childCount(const std::string& name = "") const;

    /// 查找子节点（按属性匹配）
    /// @param childName  子节点名称
    /// @param attrName   属性名
    /// @param attrValue  属性值
    /// @return 找到的节点，未找到返回无效节点
    XmlNode findChildByAttribute(const std::string& childName,
                                  const std::string& attrName,
                                  const std::string& attrValue) const;

    // 内部构造（由 XmlReader 创建）
    explicit XmlNode(detail::PugiXmlNodeWrapper* wrapper);
    detail::PugiXmlNodeWrapper* internal_() const { return wrapper_; }

private:
    detail::PugiXmlNodeWrapper* wrapper_ = nullptr;
};

/// XML 读取器（DOM 模式）
///
///   - 一次性加载整个 XML 到内存
///   - 提供根节点访问
///   - 内部基于 pugixml::xml_document
class XmlReader {
public:
    XmlReader();
    ~XmlReader();

    XmlReader(const XmlReader&) = delete;
    XmlReader& operator=(const XmlReader&) = delete;

    // -----------------------------------------------------------------------
    // 解析
    // -----------------------------------------------------------------------

    /// 从字符串解析 XML
    /// @param xml  XML 字符串（UTF-8）
    /// @return 解析成功返回 true
    bool parse(const std::string& xml);

    /// 从字节流解析 XML
    /// @param data  数据指针
    /// @param size  字节数
    /// @return 解析成功返回 true
    bool parse(const unsigned char* data, std::size_t size);

    // -----------------------------------------------------------------------
    // 访问
    // -----------------------------------------------------------------------

    /// 是否已解析
    bool isParsed() const;

    /// 获取根节点（文档根的第一个子节点，如 <?xml?> 之后的 <worksheet>）
    XmlNode root() const;

    /// 错误信息
    const std::string& error() const { return error_; }

    /// 错误偏移（解析失败位置）
    std::size_t errorOffset() const { return errorOffset_; }

private:
    detail::PugiXmlDocumentWrapper* doc_;
    bool parsed_ = false;
    std::string error_;
    std::size_t errorOffset_ = 0;
};

} // namespace ooxml
} // namespace office
} // namespace zq
