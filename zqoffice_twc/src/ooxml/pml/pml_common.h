// =============================================================================
// src/ooxml/pml/pml_common.h
//
// PresentationML 公共类型定义
//
//   PresentationML 是 OOXML 中描述 PowerPoint 演示文稿的 XML 词汇表，命名空间：
//     p:    http://schemas.openxmlformats.org/presentationml/2006/main
//     a:    http://schemas.openxmlformats.org/drawingml/2006/main
//     r:    http://schemas.openxmlformats.org/officeDocument/2006/relationships
//
//   常用元素：
//     p:presentation  - 演示文稿根节点（presentation.xml）
//     p:sldMaster     - 幻灯片母版（slideMasterN.xml）
//     p:sldLayout     - 幻灯片版式（slideLayoutN.xml）
//     p:sld           - 幻灯片（slideN.xml）
//     p:cSld          - 通用幻灯片数据（common slide data）
//     p:spTree        - 形状树（包含 sp/pic/grpSp/cxnSp/graphicFrame）
//     p:clrMap        - 颜色映射（母版到版式/幻灯片）
//     p:sldIdLst      - 幻灯片 ID 列表
//     p:sldId         - 单个幻灯片 ID（含 r:id 关系）
//     p:transition    - 幻灯片切换
//     p:timing        - 动画时间线
//
//   文档层级（继承关系）：
//     presentation.xml → slideMaster1.xml → slideLayout1.xml → slide1.xml
//     幻灯片从版式继承，版式从母版继承
// =============================================================================
#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace zq {
namespace office {
namespace ooxml {
namespace pml {

/// 幻灯片类型
enum class SlideType {
    kUnknown = 0,
    kSlide,         // p:sld
    kSlideLayout,   // p:sldLayout
    kSlideMaster,   // p:sldMaster
    kNotesSlide,    // p:notes
    kNotesMaster,   // p:notesMaster
    kHandoutMaster, // p:handoutMaster
};

/// 幻灯片类型名称
inline const char* slideTypeName(SlideType t) {
    switch (t) {
        case SlideType::kSlide:         return "sld";
        case SlideType::kSlideLayout:   return "sldLayout";
        case SlideType::kSlideMaster:   return "sldMaster";
        case SlideType::kNotesSlide:    return "notes";
        case SlideType::kNotesMaster:   return "notesMaster";
        case SlideType::kHandoutMaster: return "handoutMaster";
        case SlideType::kUnknown:
        default:                        return "unknown";
    }
}

/// 颜色映射（p:clrMap）
/// 母版的颜色方案通过 clrMap 映射到子层
struct ColorMap {
    std::string bg1;        // 通常 "lt1"
    std::string tx1;        // 通常 "dk1"
    std::string bg2;        // 通常 "lt2"
    std::string tx2;        // 通常 "dk2"
    std::string accent1;
    std::string accent2;
    std::string accent3;
    std::string accent4;
    std::string accent5;
    std::string accent6;
    std::string hlink;
    std::string folHlink;
    bool valid = false;
};

/// 幻灯片 ID 项（p:sldIdLst/p:sldId）
struct SlideId {
    std::uint32_t id = 0;        // p:sldId/@id（幻灯片 ID，非关系 ID）
    std::string relId;           // p:sldId/@r:id（关系到 slideN.xml）
};

/// 幻灯片尺寸（p:sldSz）
struct SlideSize {
    std::int64_t cx = 0;         // EMU
    std::int64_t cy = 0;         // EMU
    std::string type;            // "screen4x3" / "screen16x9" / "screen16x10" / ...
    bool valid = false;
};

} // namespace pml
} // namespace ooxml
} // namespace office
} // namespace zq
