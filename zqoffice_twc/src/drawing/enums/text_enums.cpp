// =============================================================================
// src/drawing/enums/text_enums.cpp
//
// 复杂文本枚举字符串转换实现
// 简单枚举（≤4 个值）已在头文件 inline 实现
// =============================================================================
#include "text_enums.h"

#include <unordered_map>

namespace zq {
namespace office {
namespace drawing {

// ===========================================================================
// TextVerticalType
// ===========================================================================
namespace {

struct VertEntry {
    TextVerticalType type;
    const char* name;
};

constexpr VertEntry kVertEntries[] = {
    { TextVerticalType::kHorizontal,        "horz" },
    { TextVerticalType::kVertical,          "vert" },
    { TextVerticalType::kVertical270,       "vert270" },
    { TextVerticalType::kWordArtVertical,   "wordArtVert" },
    { TextVerticalType::kEastAsianVertical, "eaVert" },
    { TextVerticalType::kMongolianVertical, "mongolianVert" },
    { TextVerticalType::kWordArtVerticalRtl,"wordArtVertRtl" },
};

const std::unordered_map<std::string, TextVerticalType>& vertStringToEnumMap() {
    static const std::unordered_map<std::string, TextVerticalType> kMap = []() {
        std::unordered_map<std::string, TextVerticalType> m;
        m.reserve(sizeof(kVertEntries) / sizeof(kVertEntries[0]));
        for (const auto& e : kVertEntries) {
            m.emplace(e.name, e.type);
        }
        return m;
    }();
    return kMap;
}

} // namespace

const char* textVerticalTypeToString(TextVerticalType t) {
    for (const auto& e : kVertEntries) {
        if (e.type == t) return e.name;
    }
    return "";
}

TextVerticalType textVerticalTypeFromString(std::string_view s) {
    if (s.empty()) return TextVerticalType::kUnknown;
    const auto& m = vertStringToEnumMap();
    auto it = m.find(std::string(s));
    if (it == m.end()) return TextVerticalType::kUnknown;
    return it->second;
}

// ===========================================================================
// TextUnderlineType
// ===========================================================================
namespace {

struct UnderlineEntry {
    TextUnderlineType type;
    const char* name;
};

constexpr UnderlineEntry kUnderlineEntries[] = {
    { TextUnderlineType::kNone,           "none" },
    { TextUnderlineType::kSingle,         "sng" },
    { TextUnderlineType::kDouble,         "dbl" },
    { TextUnderlineType::kHeavy,          "heavy" },
    { TextUnderlineType::kDotted,         "dotted" },
    { TextUnderlineType::kHeavyDotted,    "dottedHeavy" },
    { TextUnderlineType::kDash,           "dash" },
    { TextUnderlineType::kHeavyDash,      "dashHeavy" },
    { TextUnderlineType::kLongDash,       "dashLong" },
    { TextUnderlineType::kHeavyLongDash,  "dashLongHeavy" },
    { TextUnderlineType::kDotDash,        "dotDash" },
    { TextUnderlineType::kHeavyDotDash,   "dotDashHeavy" },
    { TextUnderlineType::kDotDotDash,     "dotDotDash" },
    { TextUnderlineType::kHeavyDotDotDash,"dotDotDashHeavy" },
    { TextUnderlineType::kWavy,           "wavy" },
    { TextUnderlineType::kHeavyWavy,      "wavyHeavy" },
    { TextUnderlineType::kDottedWavy,     "wavyDbl" },
};

const std::unordered_map<std::string, TextUnderlineType>& underlineStringToEnumMap() {
    static const std::unordered_map<std::string, TextUnderlineType> kMap = []() {
        std::unordered_map<std::string, TextUnderlineType> m;
        m.reserve(sizeof(kUnderlineEntries) / sizeof(kUnderlineEntries[0]));
        for (const auto& e : kUnderlineEntries) {
            m.emplace(e.name, e.type);
        }
        return m;
    }();
    return kMap;
}

} // namespace

const char* textUnderlineTypeToString(TextUnderlineType t) {
    for (const auto& e : kUnderlineEntries) {
        if (e.type == t) return e.name;
    }
    return "";
}

TextUnderlineType textUnderlineTypeFromString(std::string_view s) {
    if (s.empty()) return TextUnderlineType::kUnknown;
    const auto& m = underlineStringToEnumMap();
    auto it = m.find(std::string(s));
    if (it == m.end()) return TextUnderlineType::kUnknown;
    return it->second;
}

// ===========================================================================
// TextAlignType
// ===========================================================================
namespace {

struct AlignEntry {
    TextAlignType type;
    const char* name;
};

constexpr AlignEntry kAlignEntries[] = {
    { TextAlignType::kLeft,           "l" },
    { TextAlignType::kCenter,         "ctr" },
    { TextAlignType::kRight,          "r" },
    { TextAlignType::kJustify,        "just" },
    { TextAlignType::kDistribute,     "dist" },
    { TextAlignType::kThaiDistribute, "thaiDistribute" },
    { TextAlignType::kJustifyLow,     "justLow" },
};

const std::unordered_map<std::string, TextAlignType>& alignStringToEnumMap() {
    static const std::unordered_map<std::string, TextAlignType> kMap = []() {
        std::unordered_map<std::string, TextAlignType> m;
        m.reserve(sizeof(kAlignEntries) / sizeof(kAlignEntries[0]));
        for (const auto& e : kAlignEntries) {
            m.emplace(e.name, e.type);
        }
        return m;
    }();
    return kMap;
}

} // namespace

const char* textAlignTypeToString(TextAlignType t) {
    for (const auto& e : kAlignEntries) {
        if (e.type == t) return e.name;
    }
    return "";
}

TextAlignType textAlignTypeFromString(std::string_view s) {
    if (s.empty()) return TextAlignType::kUnknown;
    const auto& m = alignStringToEnumMap();
    auto it = m.find(std::string(s));
    if (it == m.end()) return TextAlignType::kUnknown;
    return it->second;
}

} // namespace drawing
} // namespace office
} // namespace zq
