// =============================================================================
// src/drawing/enums/theme_type.cpp
//
// ThemeType 枚举字符串转换实现
// =============================================================================
#include "theme_type.h"

#include <unordered_map>

namespace zq {
namespace office {
namespace drawing {

namespace {

struct Entry {
    ThemeType type;
    const char* name;
    bool isSysColor;
};

/// 主题类型映射表
/// 包含 12 个基础方案颜色 + 24 个变体 + 28 个系统颜色
constexpr Entry kEntries[] = {
    // --- 基础方案颜色 ---
    { ThemeType::kDark1,             "dk1",     false },
    { ThemeType::kLight1,            "lt1",     false },
    { ThemeType::kDark2,             "dk2",     false },
    { ThemeType::kLight2,            "lt2",     false },
    { ThemeType::kAccent1,           "accent1", false },
    { ThemeType::kAccent2,           "accent2", false },
    { ThemeType::kAccent3,           "accent3", false },
    { ThemeType::kAccent4,           "accent4", false },
    { ThemeType::kAccent5,           "accent5", false },
    { ThemeType::kAccent6,           "accent6", false },
    { ThemeType::kHyperlink,         "hlink",   false },
    { ThemeType::kFollowedHyperlink, "folHlink",false },

    // --- 主题变体颜色 ---
    { ThemeType::kDark1LumMod,            "dk1LumMod",       false },
    { ThemeType::kDark1LumOff,            "dk1LumOff",       false },
    { ThemeType::kLight1LumMod,           "lt1LumMod",       false },
    { ThemeType::kLight1LumOff,           "lt1LumOff",       false },
    { ThemeType::kDark2LumMod,            "dk2LumMod",       false },
    { ThemeType::kDark2LumOff,            "dk2LumOff",       false },
    { ThemeType::kLight2LumMod,           "lt2LumMod",       false },
    { ThemeType::kLight2LumOff,           "lt2LumOff",       false },
    { ThemeType::kAccent1LumMod,          "accent1LumMod",   false },
    { ThemeType::kAccent1LumOff,          "accent1LumOff",   false },
    { ThemeType::kAccent2LumMod,          "accent2LumMod",   false },
    { ThemeType::kAccent2LumOff,          "accent2LumOff",   false },
    { ThemeType::kAccent3LumMod,          "accent3LumMod",   false },
    { ThemeType::kAccent3LumOff,          "accent3LumOff",   false },
    { ThemeType::kAccent4LumMod,          "accent4LumMod",   false },
    { ThemeType::kAccent4LumOff,          "accent4LumOff",   false },
    { ThemeType::kAccent5LumMod,          "accent5LumMod",   false },
    { ThemeType::kAccent5LumOff,          "accent5LumOff",   false },
    { ThemeType::kAccent6LumMod,          "accent6LumMod",   false },
    { ThemeType::kAccent6LumOff,          "accent6LumOff",   false },
    { ThemeType::kHyperlinkLumMod,        "hlinkLumMod",     false },
    { ThemeType::kHyperlinkLumOff,        "hlinkLumOff",     false },
    { ThemeType::kFollowedHyperlinkLumMod,"folHlinkLumMod",  false },
    { ThemeType::kFollowedHyperlinkLumOff,"folHlinkLumOff",  false },

    // --- 系统颜色（sysClr）---
    { ThemeType::kSysWindowText,              "windowText",              true },
    { ThemeType::kSysWindow,                  "window",                  true },
    { ThemeType::kSysButtonFace,              "buttonFace",              true },
    { ThemeType::kSysMenuText,                "menuText",                true },
    { ThemeType::kSysMenu,                    "menu",                    true },
    { ThemeType::kSysHighlight,               "highlight",               true },
    { ThemeType::kSysHighlightText,           "highlightText",           true },
    { ThemeType::kSysCaptionText,             "captionText",             true },
    { ThemeType::kSysActiveCaption,           "activeCaption",           true },
    { ThemeType::kSysInactiveCaption,         "inactiveCaption",         true },
    { ThemeType::kSysInactiveCaptionText,     "inactiveCaptionText",     true },
    { ThemeType::kSysInfoText,                "infoText",                true },
    { ThemeType::kSysInfoBk,                  "infoBk",                  true },
    { ThemeType::kSys3DDkShadow,              "3dDkShadow",              true },
    { ThemeType::kSys3DLight,                 "3dLight",                 true },
    { ThemeType::kSysWindowFrame,             "windowFrame",             true },
    { ThemeType::kSysActiveBorder,            "activeBorder",            true },
    { ThemeType::kSysInactiveBorder,          "inactiveBorder",          true },
    { ThemeType::kSysAppWorkspace,            "appWorkspace",            true },
    { ThemeType::kSysScrollbar,               "scrollbar",               true },
    { ThemeType::kSysDesktop,                 "desktop",                 true },
    { ThemeType::kSysGradientActiveCaption,   "gradientActiveCaption",   true },
    { ThemeType::kSysGradientInactiveCaption, "gradientInactiveCaption", true },
    { ThemeType::kSysHotLight,                "hotLight",                true },
    { ThemeType::kSysMenuBar,                 "menuBar",                 true },
    { ThemeType::kSysMenuHighlight,           "menuHighlight",           true },
    { ThemeType::kSysTitleBar,                "titleBar",                true },
    { ThemeType::kSysCheckBox,                "checkBox",                true },
};

const std::unordered_map<std::string, ThemeType>& stringToEnumMap() {
    static const std::unordered_map<std::string, ThemeType> kMap = []() {
        std::unordered_map<std::string, ThemeType> m;
        m.reserve(sizeof(kEntries) / sizeof(kEntries[0]));
        for (const auto& e : kEntries) {
            m.emplace(e.name, e.type);
        }
        return m;
    }();
    return kMap;
}

} // namespace

const char* themeTypeToString(ThemeType t) {
    for (const auto& e : kEntries) {
        if (e.type == t) return e.name;
    }
    return "";
}

ThemeType themeTypeFromString(std::string_view s) {
    if (s.empty()) return ThemeType::kUnknown;
    const auto& m = stringToEnumMap();
    auto it = m.find(std::string(s));
    if (it == m.end()) return ThemeType::kUnknown;
    return it->second;
}

bool isSystemColor(ThemeType t) {
    for (const auto& e : kEntries) {
        if (e.type == t) return e.isSysColor;
    }
    return false;
}

} // namespace drawing
} // namespace office
} // namespace zq
