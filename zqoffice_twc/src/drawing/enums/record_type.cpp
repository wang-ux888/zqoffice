// =============================================================================
// src/drawing/enums/record_type.cpp
//
// RecordType 枚举字符串转换实现
// =============================================================================
#include "record_type.h"

#include <unordered_map>
#include <cstdio>

namespace zq {
namespace office {
namespace drawing {

namespace {

struct Entry {
    RecordType type;
    const char* name;
    bool isContainer;
};

/// Escher 记录类型映射表
/// 来自 [MS-ODRAW] section 2.4 RecordType Enumeration
constexpr Entry kEntries[] = {
    // --- 容器记录 ---
    { RecordType::kDggContainer,          "0xF000", true },
    { RecordType::kBStoreContainer,       "0xF001", true },
    { RecordType::kDgContainer,           "0xF002", true },
    { RecordType::kSpgrContainer,         "0xF003", true },
    { RecordType::kSpContainer,           "0xF004", true },
    { RecordType::kSolverContainer,       "0xF005", true },

    // --- 数据记录 ---
    { RecordType::kAnchor,                "0xF008", false },
    { RecordType::kSpgr,                  "0xF009", false },
    { RecordType::kSp,                    "0xF00A", false },
    { RecordType::kOpt,                   "0xF00B", false },
    { RecordType::kOpt2,                  "0xF00C", false },
    { RecordType::kClientTextbox,         "0xF00D", false },
    { RecordType::kClientAnchor,          "0xF010", false },
    { RecordType::kClientData,            "0xF011", false },
    { RecordType::kRule,                  "0xF012", false },
    { RecordType::kClientRule,            "0xF013", false },
    { RecordType::kSecondary,             "0xF018", false },
    { RecordType::kColorMRU,              "0xF01A", false },
    { RecordType::kRegroupItems,          "0xF118", false },

    // --- Shape 记录 ---
    { RecordType::kShapeFlagsAtom,        "0xF01E", false },
    { RecordType::kShapeProps,            "0xF01F", false },

    // --- Solver 记录 ---
    { RecordType::kConnectorRule,         "0xF019", false },
    { RecordType::kAlignRule,             "0xF01B", false },
    { RecordType::kArcRule,               "0xF01C", false },
    { RecordType::kCalloutRule,           "0xF01D", false },

    // --- BSE blip 子记录 ---
    { RecordType::kBlipEMF,               "0xF01A", false },
    { RecordType::kBlipWMF,               "0xF01B", false },
    { RecordType::kBlipPICT,              "0xF01C", false },
    { RecordType::kBlipJPEG,              "0xF01D", false },
    { RecordType::kBlipPNG,               "0xF01E", false },
    { RecordType::kBlipDIB,               "0xF01F", false },
    { RecordType::kBlipTIFF,              "0xF020", false },

    // --- Blip 容器 ---
    { RecordType::kBlip,                  "0xF007", false },
};

const std::unordered_map<std::uint16_t, RecordType>& codeToEnumMap() {
    static const std::unordered_map<std::uint16_t, RecordType> kMap = []() {
        std::unordered_map<std::uint16_t, RecordType> m;
        m.reserve(sizeof(kEntries) / sizeof(kEntries[0]));
        for (const auto& e : kEntries) {
            std::uint16_t code = static_cast<std::uint16_t>(e.type);
            m.emplace(code, e.type);
        }
        return m;
    }();
    return kMap;
}

} // namespace

const char* recordTypeToString(RecordType t) {
    for (const auto& e : kEntries) {
        if (e.type == t) return e.name;
    }
    return "";
}

RecordType recordTypeFromString(std::string_view s) {
    if (s.empty()) return RecordType::kUnknown;

    // 尝试 16 进制解析
    unsigned int code = 0;
    if (s.size() > 2 && (s[0] == '0') && (s[1] == 'x' || s[1] == 'X')) {
        // 16 进制
        std::string hexStr(s.substr(2));
        try {
            code = static_cast<unsigned int>(std::stoul(hexStr, nullptr, 16));
        } catch (...) {
            return RecordType::kUnknown;
        }
    } else {
        // 10 进制
        try {
            code = static_cast<unsigned int>(std::stoul(std::string(s)));
        } catch (...) {
            return RecordType::kUnknown;
        }
    }

    const auto& m = codeToEnumMap();
    auto it = m.find(static_cast<std::uint16_t>(code));
    if (it == m.end()) return RecordType::kUnknown;
    return it->second;
}

bool isContainerRecord(RecordType t) {
    for (const auto& e : kEntries) {
        if (e.type == t) return e.isContainer;
    }
    return false;
}

} // namespace drawing
} // namespace office
} // namespace zq
