// =============================================================================
// src/textlayout/textlayout_enums.h
//
// 文本布局枚举集
//
//   - BreakType                       换行类型
//   - LineBreakType                   行换行类型
//   - RunType                         Run 类型
//   - RulerType                       行规则（auto/exact/atLeast）
//   - ParagraphHorizontalAlignment    段落水平对齐
//   - ParagraphVerticalAlignment      段落垂直对齐
//   - CharacterVerticalAlignment      字符垂直对齐
//   - LayoutResult                    布局结果码
//
// 注意：drawing 命名空间已有 ParagraphVerticalAlignment / CharacterVerticalAlignment
// （对应 OOXML ST_TextFontAlignType / ST_VerticalAlignRun），textlayout 命名空间
// 定义独立的同名枚举以保持模块解耦，但语义一致。
// =============================================================================
#pragma once

#include <cstdint>
#include <string_view>

namespace zq {
namespace office {
namespace textlayout {

// ---------------------------------------------------------------------------
// BreakType（换行类型）
// 对应 OOXML ST_Br (17.18.3) + w:brk
// ---------------------------------------------------------------------------
enum class BreakType : std::uint8_t {
    kUnknown = 0,
    kLine,          // line   - 换行符（软换行）
    kPage,          // page   - 分页符
    kColumn,        // column - 分栏符
};

inline const char* breakTypeToString(BreakType t) {
    switch (t) {
        case BreakType::kLine:   return "line";
        case BreakType::kPage:   return "page";
        case BreakType::kColumn: return "column";
        case BreakType::kUnknown:
        default:                 return "";
    }
}

inline BreakType breakTypeFromString(std::string_view s) {
    if (s == "line")   return BreakType::kLine;
    if (s == "page")   return BreakType::kPage;
    if (s == "column") return BreakType::kColumn;
    return BreakType::kUnknown;
}

// ---------------------------------------------------------------------------
// LineBreakType（行换行类型）
// 描述行尾换行的方式（由 BreakType 转换而来，含段落末行标识）
// ---------------------------------------------------------------------------
enum class LineBreakType : std::uint8_t {
    kUnknown = 0,
    kNone,              // 无换行（行未满，仅段落末行）
    kSoftLine,          // 软换行（文本宽度超出，自动断行）
    kHardLine,          // 硬换行（BreakType::kLine）
    kPage,              // 分页符
    kColumn,            // 分栏符
    kParagraphEnd,      // 段落结束
};

// ---------------------------------------------------------------------------
// RunType（Run 类型）
// 描述 Run 的具体种类，用于 BaseRun::GetType()
// ---------------------------------------------------------------------------
enum class RunType : std::uint8_t {
    kUnknown = 0,
    kText,              // 文本 Run（TextRun）
    kObject,            // 对象 Run（ObjectRun，如图片占位）
    kControlLine,       // 控制符：换行
    kControlPage,       // 控制符：分页
    kControlColumn,     // 控制符：分栏
    kControlTab,        // 控制符：制表符
};

// ---------------------------------------------------------------------------
// RulerType（行规则）
// 对应 OOXML ST_LineSpacingRule (17.18.49)
// ---------------------------------------------------------------------------
enum class RulerType : std::uint8_t {
    kUnknown = 0,
    kAuto,      // auto    - 自动行距（由 ascent/descent 决定）
    kExact,     // exact   - 精确行距（固定值）
    kAtLeast,   // atLeast - 最小行距
};

inline const char* rulerTypeToString(RulerType t) {
    switch (t) {
        case RulerType::kAuto:   return "auto";
        case RulerType::kExact:  return "exact";
        case RulerType::kAtLeast:return "atLeast";
        case RulerType::kUnknown:
        default:                 return "";
    }
}

inline RulerType rulerTypeFromString(std::string_view s) {
    if (s == "auto")    return RulerType::kAuto;
    if (s == "exact")   return RulerType::kExact;
    if (s == "atLeast") return RulerType::kAtLeast;
    return RulerType::kUnknown;
}

// ---------------------------------------------------------------------------
// ParagraphHorizontalAlignment（段落水平对齐）
// 对应 OOXML ST_Jc (17.18.44) + a:pPr algn
// ---------------------------------------------------------------------------
enum class ParagraphHorizontalAlignment : std::uint8_t {
    kUnknown = 0,
    kLeft,          // left / start   - 左对齐
    kCenter,        // center / ctr   - 居中
    kRight,         // right / end    - 右对齐
    kJustify,       // both / just    - 两端对齐
    kDistribute,    // distribute     - 分散对齐
};

inline const char* paragraphHorizontalAlignmentToString(ParagraphHorizontalAlignment t) {
    switch (t) {
        case ParagraphHorizontalAlignment::kLeft:       return "left";
        case ParagraphHorizontalAlignment::kCenter:     return "center";
        case ParagraphHorizontalAlignment::kRight:      return "right";
        case ParagraphHorizontalAlignment::kJustify:    return "justify";
        case ParagraphHorizontalAlignment::kDistribute: return "distribute";
        case ParagraphHorizontalAlignment::kUnknown:
        default:                                       return "";
    }
}

inline ParagraphHorizontalAlignment paragraphHorizontalAlignmentFromString(std::string_view s) {
    if (s == "left" || s == "start")       return ParagraphHorizontalAlignment::kLeft;
    if (s == "center" || s == "ctr")       return ParagraphHorizontalAlignment::kCenter;
    if (s == "right" || s == "end")        return ParagraphHorizontalAlignment::kRight;
    if (s == "both" || s == "just" || s == "justify")  return ParagraphHorizontalAlignment::kJustify;
    if (s == "distribute")                 return ParagraphHorizontalAlignment::kDistribute;
    return ParagraphHorizontalAlignment::kUnknown;
}

// ---------------------------------------------------------------------------
// ParagraphVerticalAlignment（段落垂直对齐）
// 对应 OOXML ST_TextAnchor (drawing 命名空间下也有同名枚举)
// ---------------------------------------------------------------------------
enum class ParagraphVerticalAlignment : std::uint8_t {
    kUnknown = 0,
    kTop,           // t   - 顶部
    kCenter,        // ctr - 中间
    kBottom,        // b   - 底部
};

inline const char* paragraphVerticalAlignmentToString(ParagraphVerticalAlignment t) {
    switch (t) {
        case ParagraphVerticalAlignment::kTop:    return "t";
        case ParagraphVerticalAlignment::kCenter: return "ctr";
        case ParagraphVerticalAlignment::kBottom: return "b";
        case ParagraphVerticalAlignment::kUnknown:
        default:                                  return "";
    }
}

inline ParagraphVerticalAlignment paragraphVerticalAlignmentFromString(std::string_view s) {
    if (s == "t")   return ParagraphVerticalAlignment::kTop;
    if (s == "ctr") return ParagraphVerticalAlignment::kCenter;
    if (s == "b")   return ParagraphVerticalAlignment::kBottom;
    return ParagraphVerticalAlignment::kUnknown;
}

// ---------------------------------------------------------------------------
// CharacterVerticalAlignment（字符垂直对齐）
// 对应 OOXML ST_VerticalAlignRun (drawing 命名空间下也有同名枚举)
// ---------------------------------------------------------------------------
enum class CharacterVerticalAlignment : std::uint8_t {
    kUnknown = 0,
    kBaseline,      // baseline    - 基线
    kSuperscript,   // superscript - 上标
    kSubscript,     // subscript   - 下标
};

inline const char* characterVerticalAlignmentToString(CharacterVerticalAlignment t) {
    switch (t) {
        case CharacterVerticalAlignment::kBaseline:    return "baseline";
        case CharacterVerticalAlignment::kSuperscript: return "superscript";
        case CharacterVerticalAlignment::kSubscript:   return "subscript";
        case CharacterVerticalAlignment::kUnknown:
        default:                                       return "";
    }
}

inline CharacterVerticalAlignment characterVerticalAlignmentFromString(std::string_view s) {
    if (s == "baseline")    return CharacterVerticalAlignment::kBaseline;
    if (s == "superscript") return CharacterVerticalAlignment::kSuperscript;
    if (s == "subscript")   return CharacterVerticalAlignment::kSubscript;
    return CharacterVerticalAlignment::kUnknown;
}

// ---------------------------------------------------------------------------
// LayoutResult（布局结果码）
// TextLayout::Layout() 的返回值
// ---------------------------------------------------------------------------
enum class LayoutResult : std::uint8_t {
    kUnknown = 0,
    kSuccess,           // 布局成功
    kPageFull,          // 页面已满
    kIncomplete,        // 布局未完成（需要继续）
    kError,             // 错误
};

} // namespace textlayout
} // namespace office
} // namespace zq
