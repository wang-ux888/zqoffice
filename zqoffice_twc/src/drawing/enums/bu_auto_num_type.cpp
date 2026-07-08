// =============================================================================
// src/drawing/enums/bu_auto_num_type.cpp
//
// BuAutoNumType 枚举字符串转换实现
// =============================================================================
#include "bu_auto_num_type.h"

#include <unordered_map>

namespace zq {
namespace office {
namespace drawing {

namespace {

struct Entry {
    BuAutoNumType type;
    const char* name;
};

constexpr Entry kEntries[] = {
    { BuAutoNumType::kAlphaLcParenBoth,  "alphaLcParenBoth" },
    { BuAutoNumType::kAlphaLcParenR,     "alphaLcParenR" },
    { BuAutoNumType::kAlphaLcPeriod,     "alphaLcPeriod" },
    { BuAutoNumType::kAlphaUcParenBoth,  "alphaUcParenBoth" },
    { BuAutoNumType::kAlphaUcParenR,     "alphaUcParenR" },
    { BuAutoNumType::kAlphaUcPeriod,     "alphaUcPeriod" },
    { BuAutoNumType::kArabicParenBoth,   "arabicParenBoth" },
    { BuAutoNumType::kArabicParenR,      "arabicParenR" },
    { BuAutoNumType::kArabicPeriod,      "arabicPeriod" },
    { BuAutoNumType::kArabicPlain,       "arabicPlain" },
    { BuAutoNumType::kRomanLcParenBoth,  "romanLcParenBoth" },
    { BuAutoNumType::kRomanLcParenR,     "romanLcParenR" },
    { BuAutoNumType::kRomanLcPeriod,     "romanLcPeriod" },
    { BuAutoNumType::kRomanUcParenBoth,  "romanUcParenBoth" },
    { BuAutoNumType::kRomanUcParenR,     "romanUcParenR" },
    { BuAutoNumType::kRomanUcPeriod,     "romanUcPeriod" },
    { BuAutoNumType::kCircleLcDbdPlain,  "circleLcDbdPlain" },
    { BuAutoNumType::kCircleUcDbdPlain,  "circleUcDbdPlain" },
};

const std::unordered_map<std::string, BuAutoNumType>& stringToEnumMap() {
    static const std::unordered_map<std::string, BuAutoNumType> kMap = []() {
        std::unordered_map<std::string, BuAutoNumType> m;
        m.reserve(sizeof(kEntries) / sizeof(kEntries[0]));
        for (const auto& e : kEntries) {
            m.emplace(e.name, e.type);
        }
        return m;
    }();
    return kMap;
}

} // namespace

const char* buAutoNumTypeToString(BuAutoNumType t) {
    for (const auto& e : kEntries) {
        if (e.type == t) return e.name;
    }
    return "";
}

BuAutoNumType buAutoNumTypeFromString(std::string_view s) {
    if (s.empty()) return BuAutoNumType::kUnknown;
    const auto& m = stringToEnumMap();
    auto it = m.find(std::string(s));
    if (it == m.end()) return BuAutoNumType::kUnknown;
    return it->second;
}

} // namespace drawing
} // namespace office
} // namespace zq
