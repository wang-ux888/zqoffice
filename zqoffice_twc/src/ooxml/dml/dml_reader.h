// =============================================================================
// src/ooxml/dml/dml_reader.h
//
// DmlReader：DrawingML XML 读取器
//
//   DrawingML 是 OOXML 描述图形的统一 XML 词汇表，被多个文档类型共用：
//     - xlsx 中的 xl/drawings/drawingN.xml（命名空间前缀 xdr:）
//     - pptx 中的 ppt/slides/slideN.xml 内嵌形状树（命名空间前缀 p:）
//     - docx 中的 word/document.xml 内嵌 wps:/wpg:（命名空间前缀 wps:/wpg:）
//     - 通用：xl/charts/chartN.xml (c:)、xl/theme/themeN.xml (a:)、
//            ppt/diagrams/diagramN.xml (dgm:)
//
//   DmlReader 提供：
//     - parseSpreadsheetDrawing：解析 xdr:wsDr 根节点
//     - parseWordDrawing：       解析 wpg:wgp 或单个 wps:wsp
//     - parsePresentationDrawing：解析 p:spTree
//     - parseChart：             解析 c:chartSpace（仅返回根节点，详情由 ChartConvert 处理）
//     - parseTheme：             解析 a:theme（仅返回根节点，详情由 ThemeRuntimeBuilder 处理）
//     - parseDiagram：           解析 dgm:diagram（保留原始 XML，由上层处理）
//
//   输出：
//     - root() 返回 GroupShapeNode 树（顶层为虚拟根，子节点为 anchor 或直接形状）
//     - chartRoot() / themeRoot() 返回对应 XmlReader 的根节点
//
//   解析策略：
//     - 命名空间无关：通过 localName 匹配元素，忽略前缀
//     - 容错：未知元素不报错，跳过并继续
//     - 保留原始 XmlNode：用户可通过 xmlNode() 访问未解析字段
// =============================================================================
#pragma once

#include <memory>
#include <string>

#include "../xml_reader.h"
#include "group_shape_node.h"

namespace zq {
namespace office {
namespace ooxml {
namespace dml {

/// DrawingML 读取器
class DmlReader {
public:
    DmlReader() = default;
    ~DmlReader() = default;

    DmlReader(const DmlReader&) = delete;
    DmlReader& operator=(const DmlReader&) = delete;

    // -----------------------------------------------------------------------
    // Drawing 解析
    // -----------------------------------------------------------------------

    /// 解析 Spreadsheet drawing XML（根节点 xdr:wsDr）
    /// @return 解析成功返回 true
    bool parseSpreadsheetDrawing(const std::string& xml);

    /// 解析 Word drawing XML（顶层 wpg:wgp 或 wps:wsp）
    /// @return 解析成功返回 true
    bool parseWordDrawing(const std::string& xml);

    /// 解析 Presentation slide XML 中的形状树（p:spTree）
    /// @return 解析成功返回 true
    bool parsePresentationDrawing(const std::string& xml);

    // -----------------------------------------------------------------------
    // Chart 解析（c:chartSpace）
    // -----------------------------------------------------------------------

    bool parseChart(const std::string& xml);

    /// 获取 chart 根节点（parseChart 后可用）
    XmlNode chartRoot() const;

    // -----------------------------------------------------------------------
    // Theme 解析（a:theme）
    // -----------------------------------------------------------------------

    bool parseTheme(const std::string& xml);

    /// 获取 theme 根节点（parseTheme 后可用）
    XmlNode themeRoot() const;

    // -----------------------------------------------------------------------
    // Diagram 解析（保留原始 XML，由上层处理）
    // -----------------------------------------------------------------------

    bool parseDiagram(const std::string& xml);

    // -----------------------------------------------------------------------
    // 结果访问
    // -----------------------------------------------------------------------

    /// 获取 drawing 根节点树（parseXxxDrawing 后可用）
    /// 顶层为虚拟根节点，子节点为各个 anchor 或直接形状
    const GroupShapeNode::Ptr& root() const { return root_; }

    /// 错误信息
    const std::string& error() const { return error_; }

    /// 是否已解析
    bool isParsed() const { return parsed_; }

    /// 内部 XmlReader（用于访问 chart/theme 等原始文档）
    const XmlReader& xmlReader() const { return xmlReader_; }

private:
    // -----------------------------------------------------------------------
    // 内部解析：节点构造
    // -----------------------------------------------------------------------

    /// 通用节点解析（按 localName 分发到具体 parser）
    GroupShapeNode::Ptr parseGeneric_(const XmlNode& node);

    /// 解析 sp（kShape）
    GroupShapeNode::Ptr parseShape_(const XmlNode& node);

    /// 解析 pic（kPicture）
    GroupShapeNode::Ptr parsePicture_(const XmlNode& node);

    /// 解析 grpSp（kGroup）
    GroupShapeNode::Ptr parseGroup_(const XmlNode& node);

    /// 解析 cxnSp（kConnector）
    GroupShapeNode::Ptr parseConnector_(const XmlNode& node);

    /// 解析 graphicFrame（kGraphicFrame）
    GroupShapeNode::Ptr parseGraphicFrame_(const XmlNode& node);

    /// 解析 anchor（xdr:twoCellAnchor / xdr:oneCellAnchor / xdr:absoluteAnchor）
    GroupShapeNode::Ptr parseAnchor_(const XmlNode& node);

    // -----------------------------------------------------------------------
    // 内部解析：属性
    // -----------------------------------------------------------------------

    /// 解析 spPr / grpSpPr（共享相同结构）
    void parseSpPr_(const XmlNode& spPr, ShapeProperties& props);

    /// 解析 xfrm（a:xfrm / a:chOff+a:chExt）
    void parseXfrm_(const XmlNode& xfrm, Transform& xfrm_out);

    /// 解析 prstGeom（a:prstGeom）
    void parsePrstGeom_(const XmlNode& prstGeom, PresetGeometry& geom);

    /// 解析 fill（扫描 spPr 子节点，识别 solidFill/noFill/gradFill/...）
    void parseFill_(const XmlNode& spPr, Fill& fill);

    /// 解析 line（a:ln）
    void parseLine_(const XmlNode& ln, Line& line);

    // -----------------------------------------------------------------------
    // 工具
    // -----------------------------------------------------------------------

    /// 解析颜色节点（a:srgbClr → "RRGGBB"，a:schemeClr → "scheme:ColorName"）
    std::string parseColor_(const XmlNode& colorNode);

    /// 从带前缀的限定名提取本地名（"xdr:twoCellAnchor" → "twoCellAnchor"）
    static std::string localName_(const std::string& qualified);

    /// 节点本地名匹配（忽略命名空间前缀）
    static bool matchLocalName_(const XmlNode& node,
                                  const std::string& localName);

    /// 获取节点的本地名（不带前缀）
    static std::string nodeLocalName_(const XmlNode& node);

    // -----------------------------------------------------------------------
    // 数据
    // -----------------------------------------------------------------------

    XmlReader xmlReader_;             // chart/theme 共用
    GroupShapeNode::Ptr root_;        // drawing 树
    std::string error_;
    bool parsed_ = false;
    bool chartParsed_ = false;
    bool themeParsed_ = false;
};

} // namespace dml
} // namespace ooxml
} // namespace office
} // namespace zq
