// =============================================================================
// src/ooxml/dml/dml_common.h
//
// DrawingML 公共类型定义
//
//   DrawingML（Drawing XML）是 OOXML 中描述图形的 XML 词汇表，命名空间：
//     a:    http://schemas.openxmlformats.org/drawingml/2006/main
//     xdr:  http://schemas.openxmlformats.org/drawingml/2006/spreadsheetDrawing
//     wps:  http://schemas.microsoft.com/office/word/2010/wordprocessingShape
//     wpg:  http://schemas.microsoft.com/office/word/2010/wordprocessingGroup
//     pic:  http://schemas.openxmlformats.org/drawingml/2006/picture
//     c:    http://schemas.openxmlformats.org/drawingml/2006/chart
//
//   常用元素：
//     a:sp / a:grpSp / a:pic / a:cxnSp / a:graphicFrame
//     a:spPr (shape properties) / a:grpSpPr (group shape properties)
//     a:xfrm (transform) / a:off (offset) / a:ext (extent)
//     a:prstGeom (preset geometry) / a:custGeom (custom geometry)
//     a:solidFill / a:noFill / a:gradFill / a:blipFill / a:pattFill
//     a:ln (line) / a:effectLst (effects) / a:txBody (text body)
//
//   EMU 单位：1 inch = 914400 EMU，1 pt = 12700 EMU，1 px = 9525 EMU (96dpi)
// =============================================================================
#pragma once

#include <cstdint>
#include <string>

namespace zq {
namespace office {
namespace ooxml {
namespace dml {

/// EMU 单位常量
constexpr std::int64_t kEmuPerInch = 914400;
constexpr std::int64_t kEmuPerPoint = 12700;
constexpr std::int64_t kEmuPerPixel96 = 9525;  // 96 dpi

enum class GroupShapeNodeType {
    kUnknown = 0,
    kGroup,             // grpSp - 组形状
    kShape,             // sp - 形状
    kPicture,           // pic - 图片
    kConnector,         // cxnSp - 连接器
    kGraphicFrame,      // graphicFrame - 图形框（图表/表格）
    kAbsoluteAnchor,    // xdr:absoluteAnchor
    kOneCellAnchor,     // xdr:oneCellAnchor
    kTwoCellAnchor,     // xdr:twoCellAnchor
    kSlideTree,         // p:spTree - 幻灯片形状树
};

/// 节点类型名称（用于日志/调试）
inline const char* nodeTypeName(GroupShapeNodeType t) {
    switch (t) {
        case GroupShapeNodeType::kGroup:           return "grpSp";
        case GroupShapeNodeType::kShape:           return "sp";
        case GroupShapeNodeType::kPicture:         return "pic";
        case GroupShapeNodeType::kConnector:       return "cxnSp";
        case GroupShapeNodeType::kGraphicFrame:    return "graphicFrame";
        case GroupShapeNodeType::kAbsoluteAnchor:  return "absoluteAnchor";
        case GroupShapeNodeType::kOneCellAnchor:   return "oneCellAnchor";
        case GroupShapeNodeType::kTwoCellAnchor:   return "twoCellAnchor";
        case GroupShapeNodeType::kSlideTree:       return "spTree";
        case GroupShapeNodeType::kUnknown:
        default:                                   return "unknown";
    }
}

/// 变换信息（a:xfrm / a:chOff+a:chExt）
struct Transform {
    std::int64_t x = 0;       // EMU
    std::int64_t y = 0;
    std::int64_t cx = 0;      // width
    std::int64_t cy = 0;      // height
    std::int64_t chX = 0;     // child offset x
    std::int64_t chY = 0;
    std::int64_t chCx = 0;    // child extent cx
    std::int64_t chCy = 0;    // child extent cy
    bool hasChildOffset = false;
    bool valid = false;
};

/// 预设几何信息
struct PresetGeometry {
    std::string prst;         // preset name (e.g., "rect", "roundRect", "ellipse")
    bool valid = false;
};

/// 填充类型
enum class FillType {
    kNone = 0,
    kSolid,
    kGradient,
    kPattern,
    kBlip,
    kNoFill,
    kGroup,
};

/// 填充信息
struct Fill {
    FillType type = FillType::kNone;
    std::string color;        // solid color (RRGGBB)
    std::int32_t alpha = -1;  // 0-100000 (undefined = -1)
};

/// 线条（轮廓）
struct Line {
    bool noFill = false;
    std::string color;        // RRGGBB
    std::int32_t width = 9525;  // EMU (default 0.75pt)
    bool valid = false;
};

/// 形状属性（a:spPr）
struct ShapeProperties {
    Transform xfrm;
    PresetGeometry prstGeom;
    Fill fill;
    Line line;
    bool hasXfrm = false;
    bool hasSpPr = false;
};

} // namespace dml
} // namespace ooxml
} // namespace office
} // namespace zq
