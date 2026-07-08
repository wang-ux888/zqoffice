// =============================================================================
// src/ooxml/pml/pml_reader.h
//
// PmlReader：PresentationML XML 读取器
//
//   PresentationML 描述 PowerPoint 演示文稿的层级结构：
//     presentation.xml → slideMasterN.xml → slideLayoutN.xml → slideN.xml
//
//   PmlReader 提供：
//     - parseSlide：       解析 p:sld（单张幻灯片）
//     - parseSlideLayout： 解析 p:sldLayout（幻灯片版式）
//     - parseSlideMaster： 解析 p:sldMaster（幻灯片母版）
//     - parsePresentation：解析 p:presentation（演示文稿根）
//
//   输出：
//     - root()：返回 dml::GroupShapeNode 形状树（spTree 内容）
//     - slideType()：标识当前解析的幻灯片类型
//     - slideId()：幻灯片 ID（p:sld/@show + p:cSld/p:spTree）
//     - colorMap()：颜色映射（仅 layout/master 有）
//     - slideIds()：所有幻灯片 ID 列表（仅 presentation 有）
//     - slideSize()：幻灯片尺寸（仅 presentation 有）
//
//   注意：PmlReader 复用 dml::GroupShapeNode 表示形状树，因为
//         p:spTree 内的结构与 dml 完全一致
// =============================================================================
#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "../dml/group_shape_node.h"
#include "../xml_reader.h"
#include "pml_common.h"

namespace zq {
namespace office {
namespace ooxml {
namespace pml {

/// PresentationML 读取器
class PmlReader {
public:
    PmlReader() = default;
    ~PmlReader() = default;

    PmlReader(const PmlReader&) = delete;
    PmlReader& operator=(const PmlReader&) = delete;

    // -----------------------------------------------------------------------
    // 解析入口
    // -----------------------------------------------------------------------

    /// 解析 p:sld（幻灯片）
    bool parseSlide(const std::string& xml);

    /// 解析 p:sldLayout（幻灯片版式）
    bool parseSlideLayout(const std::string& xml);

    /// 解析 p:sldMaster（幻灯片母版）
    bool parseSlideMaster(const std::string& xml);

    /// 解析 p:presentation（演示文稿根）
    bool parsePresentation(const std::string& xml);

    // -----------------------------------------------------------------------
    // 结果访问
    // -----------------------------------------------------------------------

    /// 形状树（spTree 内容，dml::GroupShapeNode 树）
    /// 顶层为虚拟根，子节点为各个形状
    const dml::GroupShapeNode::Ptr& root() const { return root_; }

    /// 当前幻灯片类型
    SlideType slideType() const { return slideType_; }

    /// 颜色映射（仅 layout/master 有效）
    const ColorMap& colorMap() const { return colorMap_; }

    /// 幻灯片 ID 列表（仅 presentation 有效）
    const std::vector<SlideId>& slideIds() const { return slideIds_; }

    /// 幻灯片尺寸（仅 presentation 有效）
    const SlideSize& slideSize() const { return slideSize_; }

    /// 错误信息
    const std::string& error() const { return error_; }

    /// 是否已解析
    bool isParsed() const { return parsed_; }

    /// 内部 XmlReader（用于访问未结构化的字段）
    const XmlReader& xmlReader() const { return xmlReader_; }

private:
    // -----------------------------------------------------------------------
    // 内部解析
    // -----------------------------------------------------------------------

    /// 通用解析：从 XML 根节点定位 cSld，提取 spTree
    /// @param root  XML 根节点（p:sld / p:sldLayout / p:sldMaster）
    /// @param type  幻灯片类型
    bool parseSlideLike_(const XmlNode& root, SlideType type);

    /// 解析 p:clrMap
    void parseColorMap_(const XmlNode& clrMap);

    /// 解析 p:sldSz
    void parseSlideSize_(const XmlNode& sldSz);

    /// 解析 p:sldIdLst
    void parseSlideIdLst_(const XmlNode& sldIdLst);

    // -----------------------------------------------------------------------
    // 工具
    // -----------------------------------------------------------------------

    static std::string localName_(const std::string& qualified);
    static std::string nodeLocalName_(const XmlNode& node);

    // -----------------------------------------------------------------------
    // 数据
    // -----------------------------------------------------------------------

    XmlReader xmlReader_;
    dml::GroupShapeNode::Ptr root_;
    SlideType slideType_ = SlideType::kUnknown;
    ColorMap colorMap_;
    std::vector<SlideId> slideIds_;
    SlideSize slideSize_;
    std::string error_;
    bool parsed_ = false;
};

} // namespace pml
} // namespace ooxml
} // namespace office
} // namespace zq
