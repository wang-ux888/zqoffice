// =============================================================================
// src/drawing/enums/text_enums.h
//
// 文本相关枚举集
//
//   对应 OOXML ECMA-376 中 ST_Text* 系列枚举：
//     - TextAnchoringType  (20.1.10.59) - 文本垂直锚定
//     - TextVertOverflowType (20.1.10.83) - 文本垂直溢出
//     - TextHorzOverflowType (20.1.10.41) - 文本水平溢出
//     - TextWrapType (20.1.10.84) - 文本换行
//     - TextVerticalType (20.1.10.82) - 文本方向
//     - TextFitType (20.1.10.40) - 文本适应方式
//     - TextUnderlineType (20.1.10.81) - 下划线
//     - TextStrikeType (20.1.10.80) - 删除线
//     - TextCapsType (20.1.10.31) - 大小写
//     - TextAlignType (20.1.10.58) - 水平对齐
//     - ParagraphVerticalAlignment - 段落垂直对齐
//     - CharacterVerticalAlignment - 字符垂直对齐
//
//   简单枚举（≤4 个值）的转换函数使用 inline 实现；
//   复杂枚举的转换函数在 .cpp 中实现。
// =============================================================================
#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace zq {
namespace office {
namespace drawing {

// ---------------------------------------------------------------------------
// TextAnchoringType（文本垂直锚定）
// 对应 OOXML ST_TextAnchoringType (20.1.10.59)
// 用于 a:bodyPr anchor="" 属性
// ---------------------------------------------------------------------------
enum class TextAnchoringType : std::uint8_t {
    kUnknown = 0,
    kTop,       // t   - 顶部
    kCenter,    // ctr - 中间
    kBottom,    // b   - 底部
};

inline const char* textAnchoringTypeToString(TextAnchoringType t) {
    switch (t) {
        case TextAnchoringType::kTop:    return "t";
        case TextAnchoringType::kCenter: return "ctr";
        case TextAnchoringType::kBottom: return "b";
        case TextAnchoringType::kUnknown:
        default:                         return "";
    }
}

inline TextAnchoringType textAnchoringTypeFromString(std::string_view s) {
    if (s == "t")   return TextAnchoringType::kTop;
    if (s == "ctr") return TextAnchoringType::kCenter;
    if (s == "b")   return TextAnchoringType::kBottom;
    return TextAnchoringType::kUnknown;
}

// ---------------------------------------------------------------------------
// TextVertOverflowType（文本垂直溢出）
// 对应 OOXML ST_TextVertOverflowType (20.1.10.83)
// 用于 a:bodyPr vertOverflow="" 属性
// ---------------------------------------------------------------------------
enum class TextVertOverflowType : std::uint8_t {
    kUnknown = 0,
    kClip,      // clip      - 裁剪
    kEllipsis,  // ellipsis  - 省略号
    kOverflow,  // overflow  - 溢出
};

inline const char* textVertOverflowTypeToString(TextVertOverflowType t) {
    switch (t) {
        case TextVertOverflowType::kClip:     return "clip";
        case TextVertOverflowType::kEllipsis: return "ellipsis";
        case TextVertOverflowType::kOverflow: return "overflow";
        case TextVertOverflowType::kUnknown:
        default:                              return "";
    }
}

inline TextVertOverflowType textVertOverflowTypeFromString(std::string_view s) {
    if (s == "clip")     return TextVertOverflowType::kClip;
    if (s == "ellipsis") return TextVertOverflowType::kEllipsis;
    if (s == "overflow") return TextVertOverflowType::kOverflow;
    return TextVertOverflowType::kUnknown;
}

// ---------------------------------------------------------------------------
// TextHorzOverflowType（文本水平溢出）
// 对应 OOXML ST_TextHorzOverflowType (20.1.10.41)
// 用于 a:bodyPr horzOverflow="" 属性
// ---------------------------------------------------------------------------
enum class TextHorzOverflowType : std::uint8_t {
    kUnknown = 0,
    kClip,      // clip     - 裁剪
    kOverflow,  // overflow - 溢出
};

inline const char* textHorzOverflowTypeToString(TextHorzOverflowType t) {
    switch (t) {
        case TextHorzOverflowType::kClip:     return "clip";
        case TextHorzOverflowType::kOverflow: return "overflow";
        case TextHorzOverflowType::kUnknown:
        default:                              return "";
    }
}

inline TextHorzOverflowType textHorzOverflowTypeFromString(std::string_view s) {
    if (s == "clip")     return TextHorzOverflowType::kClip;
    if (s == "overflow") return TextHorzOverflowType::kOverflow;
    return TextHorzOverflowType::kUnknown;
}

// ---------------------------------------------------------------------------
// TextWrapType（文本换行）
// 对应 OOXML ST_TextWrappingType (20.1.10.84)
// 用于 a:bodyPr wrap="" 属性
// ---------------------------------------------------------------------------
enum class TextWrapType : std::uint8_t {
    kUnknown = 0,
    kNone,      // none    - 不换行
    kSquare,    // square  - 矩形换行
};

inline const char* textWrapTypeToString(TextWrapType t) {
    switch (t) {
        case TextWrapType::kNone:   return "none";
        case TextWrapType::kSquare: return "square";
        case TextWrapType::kUnknown:
        default:                    return "";
    }
}

inline TextWrapType textWrapTypeFromString(std::string_view s) {
    if (s == "none")   return TextWrapType::kNone;
    if (s == "square") return TextWrapType::kSquare;
    return TextWrapType::kUnknown;
}

// ---------------------------------------------------------------------------
// TextVerticalType（文本方向）
// 对应 OOXML ST_TextVerticalType (20.1.10.82)
// 用于 a:bodyPr vert="" 属性
// ---------------------------------------------------------------------------
enum class TextVerticalType : std::uint8_t {
    kUnknown = 0,
    kHorizontal,       // horz          - 水平
    kVertical,         // vert          - 垂直（从上到下，从右到左）
    kVertical270,      // vert270       - 垂直 270 度
    kWordArtVertical,  // wordArtVert   - 艺术字垂直
    kEastAsianVertical,// eaVert        - 东亚垂直
    kMongolianVertical,// mongolianVert - 蒙古文垂直
    kWordArtVerticalRtl,//wordArtVertRtl- 艺术字垂直（从右到左）
};

const char* textVerticalTypeToString(TextVerticalType t);
TextVerticalType textVerticalTypeFromString(std::string_view s);

// ---------------------------------------------------------------------------
// TextFitType（文本适应方式）
// 对应 OOXML a:bodyPr 子元素 a:spAutoFit / a:normAutofit / a:noAutofit
// ---------------------------------------------------------------------------
enum class TextFitType : std::uint8_t {
    kUnknown = 0,
    kNoAutofit,      // noAutofit   - 不自动适应
    kNormalAutofit,  // normAutofit - 普通自动适应（缩放字号）
    kShapeAutofit,   // spAutoFit   - 形状自动适应（改变形状大小）
};

inline const char* textFitTypeToString(TextFitType t) {
    switch (t) {
        case TextFitType::kNoAutofit:     return "noAutofit";
        case TextFitType::kNormalAutofit: return "normAutofit";
        case TextFitType::kShapeAutofit:  return "spAutoFit";
        case TextFitType::kUnknown:
        default:                          return "";
    }
}

inline TextFitType textFitTypeFromString(std::string_view s) {
    if (s == "noAutofit")   return TextFitType::kNoAutofit;
    if (s == "normAutofit") return TextFitType::kNormalAutofit;
    if (s == "spAutoFit")   return TextFitType::kShapeAutofit;
    return TextFitType::kUnknown;
}

// ---------------------------------------------------------------------------
// TextUnderlineType（下划线类型）
// 对应 OOXML ST_TextUnderlineType (20.1.10.81)
// 用于 a:rPr u="" 属性
// ---------------------------------------------------------------------------
enum class TextUnderlineType : std::uint8_t {
    kUnknown = 0,
    kNone,           // none
    kSingle,         // sng       - 单线
    kDouble,         // dbl       - 双线
    kHeavy,          // heavy     - 粗线
    kDotted,         // dotted    - 点线
    kHeavyDotted,    // dottedHeavy - 粗点线
    kDash,           // dash      - 虚线
    kHeavyDash,      // dashHeavy - 粗虚线
    kLongDash,       // dashLong  - 长虚线
    kHeavyLongDash,  // dashLongHeavy - 粗长虚线
    kDotDash,        // dotDash   - 点划线
    kHeavyDotDash,   // dotDashHeavy - 粗点划线
    kDotDotDash,     // dotDotDash - 双点划线
    kHeavyDotDotDash,// dotDotDashHeavy - 粗双点划线
    kWavy,           // wavy      - 波浪线
    kHeavyWavy,      // wavyHeavy - 粗波浪线
    kDottedWavy,     // wavyDbl   - 双波浪线
};

const char* textUnderlineTypeToString(TextUnderlineType t);
TextUnderlineType textUnderlineTypeFromString(std::string_view s);

// ---------------------------------------------------------------------------
// TextStrikeType（删除线类型）
// 对应 OOXML ST_TextStrikeType (20.1.10.80)
// 用于 a:rPr strike="" 属性
// ---------------------------------------------------------------------------
enum class TextStrikeType : std::uint8_t {
    kUnknown = 0,
    kNone,           // noStrike
    kSingle,         // sngStrike
    kDouble,         // dblStrike
};

inline const char* textStrikeTypeToString(TextStrikeType t) {
    switch (t) {
        case TextStrikeType::kNone:   return "noStrike";
        case TextStrikeType::kSingle: return "sngStrike";
        case TextStrikeType::kDouble: return "dblStrike";
        case TextStrikeType::kUnknown:
        default:                      return "";
    }
}

inline TextStrikeType textStrikeTypeFromString(std::string_view s) {
    if (s == "noStrike")  return TextStrikeType::kNone;
    if (s == "sngStrike") return TextStrikeType::kSingle;
    if (s == "dblStrike") return TextStrikeType::kDouble;
    return TextStrikeType::kUnknown;
}

// ---------------------------------------------------------------------------
// TextCapsType（大小写）
// 对应 OOXML ST_TextCapsType (20.1.10.31)
// 用于 a:rPr cap="" 属性
// ---------------------------------------------------------------------------
enum class TextCapsType : std::uint8_t {
    kUnknown = 0,
    kNone,       // none
    kSmall,      // small - 小型大写
    kAll,        // all   - 全部大写
};

inline const char* textCapsTypeToString(TextCapsType t) {
    switch (t) {
        case TextCapsType::kNone:  return "none";
        case TextCapsType::kSmall: return "small";
        case TextCapsType::kAll:   return "all";
        case TextCapsType::kUnknown:
        default:                   return "";
    }
}

inline TextCapsType textCapsTypeFromString(std::string_view s) {
    if (s == "none")  return TextCapsType::kNone;
    if (s == "small") return TextCapsType::kSmall;
    if (s == "all")   return TextCapsType::kAll;
    return TextCapsType::kUnknown;
}

// ---------------------------------------------------------------------------
// TextAlignType（水平对齐）
// 对应 OOXML ST_TextAlignType (20.1.10.58)
// 用于 a:pPr algn="" 属性
// ---------------------------------------------------------------------------
enum class TextAlignType : std::uint8_t {
    kUnknown = 0,
    kLeft,        // l   - 左对齐
    kCenter,      // ctr - 居中
    kRight,       // r   - 右对齐
    kJustify,     // just- 两端对齐
    kDistribute,  // dist- 分散对齐
    kThaiDistribute, // thaiDistribute - 泰语分散对齐
    kJustifyLow,  // justLow - 低两端对齐
};

const char* textAlignTypeToString(TextAlignType t);
TextAlignType textAlignTypeFromString(std::string_view s);

// ---------------------------------------------------------------------------
// ParagraphVerticalAlignment（段落垂直对齐）
// 对应 OOXML ST_TextFontAlignType (20.1.10.67)
// 用于 a:pPr fontAlgn="" 属性
// ---------------------------------------------------------------------------
enum class ParagraphVerticalAlignment : std::uint8_t {
    kUnknown = 0,
    kTop,           // t     - 顶部
    kCenter,        // ctr   - 中间
    kBaseline,      // base  - 基线
    kBottom,        // b     - 底部
    kAuto,          // auto  - 自动
};

inline const char* paragraphVerticalAlignmentToString(ParagraphVerticalAlignment t) {
    switch (t) {
        case ParagraphVerticalAlignment::kTop:      return "t";
        case ParagraphVerticalAlignment::kCenter:   return "ctr";
        case ParagraphVerticalAlignment::kBaseline: return "base";
        case ParagraphVerticalAlignment::kBottom:   return "b";
        case ParagraphVerticalAlignment::kAuto:     return "auto";
        case ParagraphVerticalAlignment::kUnknown:
        default:                                    return "";
    }
}

inline ParagraphVerticalAlignment paragraphVerticalAlignmentFromString(std::string_view s) {
    if (s == "t")    return ParagraphVerticalAlignment::kTop;
    if (s == "ctr")  return ParagraphVerticalAlignment::kCenter;
    if (s == "base") return ParagraphVerticalAlignment::kBaseline;
    if (s == "b")    return ParagraphVerticalAlignment::kBottom;
    if (s == "auto") return ParagraphVerticalAlignment::kAuto;
    return ParagraphVerticalAlignment::kUnknown;
}

// ---------------------------------------------------------------------------
// CharacterVerticalAlignment（字符垂直对齐）
// 对应 WordprocessingML ST_VerticalAlignRun
// 用于 w:rPr w:vertAlign="" 属性
// ---------------------------------------------------------------------------
enum class CharacterVerticalAlignment : std::uint8_t {
    kUnknown = 0,
    kBaseline,      // baseline   - 基线
    kSuperscript,   // superscript- 上标
    kSubscript,     // subscript  - 下标
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

} // namespace drawing
} // namespace office
} // namespace zq
