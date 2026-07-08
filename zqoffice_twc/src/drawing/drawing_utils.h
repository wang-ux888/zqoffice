// =============================================================================
// src/drawing/drawing_utils.h
//
// DrawingUtils 类：Drawing 转换工具集
//
//   提供二进制 Escher 记录与统一枚举之间的映射转换，以及主题颜色应用工具。
//   主要功能：
//     1. 静态映射表（二进制编码 ↔ 枚举）：
//        - prst_shape_type_map_  : MSO_AUTO_SHAPE_TYPE 二进制码 ↔ PrstShapeType
//        - prst_clr_type_map03_  : Office 2003 颜色索引 ↔ PrstClrType
//        - prst_clr_type_map07_  : Office 2007 颜色索引 ↔ PrstClrType
//        - bu_auto_num_type_map_ : 二进制 BuAutoNum 编码 ↔ BuAutoNumType
//     2. UseHlinkClr：将主题超链接颜色应用到文本运行
//
//   二进制编码来源：
//     - [MS-ODRAW] section 2.4 : OfficeArtRecordHeader + OPT 记录属性
//     - [MS-OVBA]  : MSO_AUTO_SHAPE_TYPE 枚举
//     - MSO_COLOR_ENUM : 旧版调色板颜色索引
// =============================================================================
#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "drawing/enums/prst_shape_type.h"
#include "drawing/enums/prst_clr_type.h"
#include "drawing/enums/bu_auto_num_type.h"
#include "drawing/enums/theme_type.h"

namespace zq {
namespace office {
namespace drawing {

class Theme;
class TextRun;

/// Drawing 转换工具集
///
class DrawingUtils {
public:
    // -----------------------------------------------------------------------
    // 二进制码 ↔ 枚举 转换
    // -----------------------------------------------------------------------

    /// MSO_AUTO_SHAPE_TYPE 二进制码 → PrstShapeType
    /// @param msoType  MSO 形状类型码（来自 Escher OPT 记录 PERSIST_ID 0x0080）
    /// @return 对应的 PrstShapeType，未识别返回 kUnknown
    static PrstShapeType msoShapeTypeToPrst(std::uint32_t msoType);

    /// PrstShapeType → MSO_AUTO_SHAPE_TYPE 二进制码
    /// @return 二进制码，未识别返回 0
    static std::uint32_t prstToMsoShapeType(PrstShapeType type);

    /// Office 2003 颜色索引 → PrstClrType
    /// @param idx  旧版二进制颜色索引
    /// @return 对应的 PrstClrType，未识别返回 kUnknown
    static PrstClrType colorIndex03ToPrstClr(std::uint16_t idx);

    /// Office 2007 颜色索引 → PrstClrType
    /// @param idx  Escher OPT 颜色索引
    /// @return 对应的 PrstClrType，未识别返回 kUnknown
    static PrstClrType colorIndex07ToPrstClr(std::uint16_t idx);

    /// 二进制 BuAutoNum 编码 → BuAutoNumType
    /// @param code  二进制自动编号编码
    /// @return 对应的 BuAutoNumType，未识别返回 kUnknown
    static BuAutoNumType buAutoNumCodeToType(std::uint16_t code);

    /// BuAutoNumType → 二进制编码
    /// @return 二进制编码，未识别返回 0
    static std::uint16_t buAutoNumTypeToCode(BuAutoNumType type);

    // -----------------------------------------------------------------------
    // 主题颜色应用
    // -----------------------------------------------------------------------

    /// 将主题超链接颜色应用到文本运行
    ///
    ///   当文本运行设置了超链接样式时，从主题中获取 hlink 颜色并应用到 run.fontColor。
    ///   如果主题中没有 hlink 颜色，使用默认蓝色 (#0563C1)。
    ///
    /// @param run   文本运行
    /// @param theme 主题节点（可为 nullptr，此时使用默认蓝色）
    /// @return 实际使用的超链接颜色 RGB（6 位十六进制，如 "0563c1"）
    static std::string useHlinkClr(TextRun& run,
                                    const std::shared_ptr<Theme>& theme);

    // -----------------------------------------------------------------------
    // 颜色工具
    // -----------------------------------------------------------------------

    /// 将 ThemeType 映射到主题调色板索引（0-11）
    /// dk1=0, lt1=1, dk2=2, lt2=3, accent1=4, ..., accent6=9, hlink=10, folHlink=11
    /// @return 索引（0-11），非方案颜色返回 0xFF
    static std::uint8_t themeTypeToPaletteIndex(ThemeType type);

    /// 将主题调色板索引（0-11）映射到 ThemeType
    static ThemeType paletteIndexToThemeType(std::uint8_t idx);

    /// 默认超链接颜色 RGB（6 位十六进制小写）
    static const char* defaultHlinkColor();
};

} // namespace drawing
} // namespace office
} // namespace zq
