// =============================================================================
// src/drawing/enums/theme_type.h
//
// 主题颜色类型枚举（ThemeType / ThemeColorType）
//
//   对应 OOXML ECMA-376 第 20.1.6.2 节 ST_SystemColorVal 第 21.2.3.36 节
//   ST_SchemeColorVal，描述主题中的颜色槽位（color slot）：
//     - 系统颜色（windowText, window, etc.）
//     - 方案颜色（dk1, lt1, dk2, lt2, accent1-6, hlink, folHlink）
//     - 主题变体颜色（dk1LumMod/Off, accent1LumMod/Off, etc.）
//
//   用于 a:schemeClr val="accent1" / a:sysClr val="windowText" 等场景。
// =============================================================================
#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace zq {
namespace office {
namespace drawing {

/// 主题颜色槽位类型
///
/// 与 OOXML ST_SchemeColorVal 对齐（ECMA-376 第 21.2.3.36 节）
/// 12 个基础方案颜色 + 系统颜色 + 主题变体
enum class ThemeType : std::uint8_t {
    kUnknown = 0,

    // --- 基础方案颜色（12 个，ECMA-376 第 20.1.6.2 节 clrScheme）---
    kDark1,         // dk1   - 深色 1（通常 windowText）
    kLight1,        // lt1   - 浅色 1（通常 window）
    kDark2,         // dk2   - 深色 2
    kLight2,        // lt2   - 浅色 2
    kAccent1,       // accent1 - 强调色 1
    kAccent2,       // accent2 - 强调色 2
    kAccent3,       // accent3 - 强调色 3
    kAccent4,       // accent4 - 强调色 4
    kAccent5,       // accent5 - 强调色 5
    kAccent6,       // accent6 - 强调色 6
    kHyperlink,     // hlink - 超链接
    kFollowedHyperlink, // folHlink - 已访问超链接

    kDark1LumMod,       // dk1 亮度调制
    kDark1LumOff,       // dk1 亮度偏移
    kLight1LumMod,      // lt1 亮度调制
    kLight1LumOff,      // lt1 亮度偏移
    kDark2LumMod,       // dk2 亮度调制
    kDark2LumOff,       // dk2 亮度偏移
    kLight2LumMod,      // lt2 亮度调制
    kLight2LumOff,      // lt2 亮度偏移
    kAccent1LumMod,
    kAccent1LumOff,
    kAccent2LumMod,
    kAccent2LumOff,
    kAccent3LumMod,
    kAccent3LumOff,
    kAccent4LumMod,
    kAccent4LumOff,
    kAccent5LumMod,
    kAccent5LumOff,
    kAccent6LumMod,
    kAccent6LumOff,
    kHyperlinkLumMod,
    kHyperlinkLumOff,
    kFollowedHyperlinkLumMod,
    kFollowedHyperlinkLumOff,

    // --- 系统颜色（ECMA-376 第 20.1.10.78 节 ST_SystemColorVal 简化版）---
    kSysWindowText,
    kSysWindow,
    kSysButtonFace,
    kSysMenuText,
    kSysMenu,
    kSysHighlight,
    kSysHighlightText,
    kSysCaptionText,
    kSysActiveCaption,
    kSysInactiveCaption,
    kSysInactiveCaptionText,
    kSysInfoText,
    kSysInfoBk,
    kSys3DDkShadow,
    kSys3DLight,
    kSysWindowFrame,
    kSysActiveBorder,
    kSysInactiveBorder,
    kSysAppWorkspace,
    kSysScrollbar,
    kSysDesktop,
    kSysGradientActiveCaption,
    kSysGradientInactiveCaption,
    kSysHotLight,
    kSysMenuBar,
    kSysMenuHighlight,
    kSysTitleBar,
    kSysCheckBox,
};

/// 枚举 → OOXML 字符串名称（schemeClr val 值）
const char* themeTypeToString(ThemeType t);

/// 字符串 → 枚举（schemeClr / sysClr val 解析）
ThemeType themeTypeFromString(std::string_view s);

/// 是否为系统颜色（sysClr）
bool isSystemColor(ThemeType t);

} // namespace drawing
} // namespace office
} // namespace zq
